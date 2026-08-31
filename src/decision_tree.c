#include "spatter/decision_tree.h"

TreeTrainConfig tree_train_config_default(size_t num_classes)
{
    TreeTrainConfig config = {
        .max_depth = 12,
        .min_samples_leaf = 2,
        .min_samples_split = 4,
        .min_info_gain = 1e-4,
        .smoothing_lambda = 0.85,
        .num_classes = num_classes
    };
    return config;
}

Distribution distribution_create(size_t num_classes)
{
    Distribution dist;
    dist.num_classes = num_classes;
    dist.probs = (double*)spatter_calloc(num_classes, sizeof(double));
    dist.log_probs = (double*)spatter_alloc(num_classes * sizeof(double));
    for (size_t i = 0; i < num_classes; i++) {
        dist.log_probs[i] = SPATTER_NEG_INF;
    }
    return dist;
}

Distribution distribution_copy(Distribution src)
{
    Distribution dist = distribution_create(src.num_classes);
    for (size_t i = 0; i < src.num_classes; i++) {
        dist.probs[i] = src.probs[i];
        dist.log_probs[i] = src.log_probs[i];
    }
    return dist;
}

void distribution_free(Distribution dist)
{
    spatter_free(dist.probs);
    spatter_free(dist.log_probs);
}

Distribution distribution_from_counts(const double* counts, size_t num_classes, double smoothing_alpha)
{
    Distribution dist = distribution_create(num_classes);
    double total_count = 0.0;
    for (size_t i = 0; i < num_classes; i++) {
        total_count += counts[i] + smoothing_alpha;
    }
    if (total_count <= SPATTER_EPSILON) {
        double uniform = 1.0 / (double)num_classes;
        double log_uniform = log(uniform);
        for (size_t i = 0; i < num_classes; i++) {
            dist.probs[i] = uniform;
            dist.log_probs[i] = log_uniform;
        }
        return dist;
    }
    for (size_t i = 0; i < num_classes; i++) {
        double p = (counts[i] + smoothing_alpha) / total_count;
        dist.probs[i] = p;
        dist.log_probs[i] = (p > SPATTER_EPSILON) ? log(p) : SPATTER_NEG_INF;
    }
    return dist;
}

Distribution distribution_interpolate(Distribution leaf_dist, Distribution parent_dist, double lambda)
{
    size_t num_classes = leaf_dist.num_classes;
    Distribution dist = distribution_create(num_classes);
    for (size_t i = 0; i < num_classes; i++) {
        double p_leaf = leaf_dist.probs ? leaf_dist.probs[i] : (1.0 / (double)num_classes);
        double p_parent = (parent_dist.probs && parent_dist.num_classes == num_classes) ? parent_dist.probs[i] : (1.0 / (double)num_classes);
        double p = lambda * p_leaf + (1.0 - lambda) * p_parent;
        dist.probs[i] = p;
        dist.log_probs[i] = (p > SPATTER_EPSILON) ? log(p) : SPATTER_NEG_INF;
    }
    return dist;
}

double calculate_entropy(const double* counts, size_t num_classes, double total_weight)
{
    if (total_weight <= SPATTER_EPSILON) {
        return 0.0;
    }
    double entropy = 0.0;
    for (size_t i = 0; i < num_classes; i++) {
        if (counts[i] > SPATTER_EPSILON) {
            double p = counts[i] / total_weight;
            entropy -= p * (log(p) / log(2.0));
        }
    }
    return entropy;
}

const DecisionTree* decision_tree_create_leaf(Distribution dist, size_t num_samples, double entropy)
{
    DecisionTree* tree = (DecisionTree*)spatter_alloc(sizeof(DecisionTree));
    tree->is_leaf = true;
    tree->num_samples = num_samples;
    tree->entropy = entropy;
    tree->true_branch = NULL;
    tree->false_branch = NULL;
    tree->distribution = dist;
    memset(&tree->question, 0, sizeof(Question));
    return tree;
}

const DecisionTree* decision_tree_create_split(Question q, const DecisionTree* true_br, const DecisionTree* false_br, size_t num_samples, double entropy)
{
    DecisionTree* tree = (DecisionTree*)spatter_alloc(sizeof(DecisionTree));
    tree->is_leaf = false;
    tree->num_samples = num_samples;
    tree->entropy = entropy;
    tree->question = q;
    tree->true_branch = true_br;
    tree->false_branch = false_br;
    tree->distribution.num_classes = 0;
    tree->distribution.probs = NULL;
    tree->distribution.log_probs = NULL;
    return tree;
}

const DecisionTree* decision_tree_train(const TrainingSample* samples, size_t num_samples, const Question* candidate_questions, size_t num_questions, TreeTrainConfig config, size_t current_depth, Distribution parent_dist)
{
    size_t num_classes = config.num_classes;
    double* counts = (double*)spatter_calloc(num_classes, sizeof(double));
    double total_weight = 0.0;
    for (size_t i = 0; i < num_samples; i++) {
        int c = samples[i].target_class;
        if (c >= 0 && (size_t)c < num_classes) {
            double w = (samples[i].weight > 0.0) ? samples[i].weight : 1.0;
            counts[c] += w;
            total_weight += w;
        }
    }

    double entropy = calculate_entropy(counts, num_classes, total_weight);
    Distribution raw_dist = distribution_from_counts(counts, num_classes, 0.01);
    Distribution current_dist = (parent_dist.probs != NULL) ? distribution_interpolate(raw_dist, parent_dist, config.smoothing_lambda) : raw_dist;
    if (parent_dist.probs != NULL) {
        distribution_free(raw_dist);
    }
    spatter_free(counts);

    if (num_samples < config.min_samples_split || current_depth >= config.max_depth || entropy <= SPATTER_EPSILON || num_questions == 0) {
        return decision_tree_create_leaf(current_dist, num_samples, entropy);
    }

    double best_gain = 0.0;
    size_t best_q_idx = (size_t)-1;
    size_t best_yes_count = 0;
    size_t best_no_count = 0;

    for (size_t q_idx = 0; q_idx < num_questions; q_idx++) {
        Question q = candidate_questions[q_idx];
        double* yes_counts = (double*)spatter_calloc(num_classes, sizeof(double));
        double* no_counts = (double*)spatter_calloc(num_classes, sizeof(double));
        double yes_weight = 0.0;
        double no_weight = 0.0;
        size_t yes_count = 0;
        size_t no_count = 0;

        for (size_t i = 0; i < num_samples; i++) {
            double w = (samples[i].weight > 0.0) ? samples[i].weight : 1.0;
            int c = samples[i].target_class;
            if (question_eval(q, &samples[i].context)) {
                if (c >= 0 && (size_t)c < num_classes) {
                    yes_counts[c] += w;
                }
                yes_weight += w;
                yes_count++;
            } else {
                if (c >= 0 && (size_t)c < num_classes) {
                    no_counts[c] += w;
                }
                no_weight += w;
                no_count++;
            }
        }

        if (yes_count >= config.min_samples_leaf && no_count >= config.min_samples_leaf) {
            double yes_entropy = calculate_entropy(yes_counts, num_classes, yes_weight);
            double no_entropy = calculate_entropy(no_counts, num_classes, no_weight);
            double weighted_entropy = (yes_weight / total_weight) * yes_entropy + (no_weight / total_weight) * no_entropy;
            double gain = entropy - weighted_entropy;

            if (gain > best_gain) {
                best_gain = gain;
                best_q_idx = q_idx;
                best_yes_count = yes_count;
                best_no_count = no_count;
            }
        }

        spatter_free(yes_counts);
        spatter_free(no_counts);
    }

    if (best_gain < config.min_info_gain || best_q_idx == (size_t)-1) {
        return decision_tree_create_leaf(current_dist, num_samples, entropy);
    }

    Question best_q = candidate_questions[best_q_idx];
    TrainingSample* yes_samples = (TrainingSample*)spatter_alloc(best_yes_count * sizeof(TrainingSample));
    TrainingSample* no_samples = (TrainingSample*)spatter_alloc(best_no_count * sizeof(TrainingSample));
    size_t yes_i = 0;
    size_t no_i = 0;

    for (size_t i = 0; i < num_samples; i++) {
        if (question_eval(best_q, &samples[i].context)) {
            yes_samples[yes_i++] = samples[i];
        } else {
            no_samples[no_i++] = samples[i];
        }
    }

    const DecisionTree* true_br = decision_tree_train(yes_samples, best_yes_count, candidate_questions, num_questions, config, current_depth + 1, current_dist);
    const DecisionTree* false_br = decision_tree_train(no_samples, best_no_count, candidate_questions, num_questions, config, current_depth + 1, current_dist);

    spatter_free(yes_samples);
    spatter_free(no_samples);
    distribution_free(current_dist);

    return decision_tree_create_split(best_q, true_br, false_br, num_samples, entropy);
}

Distribution decision_tree_predict(const DecisionTree* tree, const Context* ctx)
{
    if (!tree) {
        Distribution empty = { .num_classes = 0, .probs = NULL, .log_probs = NULL };
        return empty;
    }
    if (tree->is_leaf) {
        return tree->distribution;
    }
    if (question_eval(tree->question, ctx)) {
        return decision_tree_predict(tree->true_branch, ctx);
    } else {
        return decision_tree_predict(tree->false_branch, ctx);
    }
}

void decision_tree_print(const DecisionTree* tree, int indent, FILE* stream)
{
    if (!tree) {
        return;
    }
    for (int i = 0; i < indent; i++) {
        fprintf(stream, "  ");
    }
    if (tree->is_leaf) {
        fprintf(stream, "LEAF (samples: %zu, entropy: %.4f):", tree->num_samples, tree->entropy);
        for (size_t c = 0; c < tree->distribution.num_classes; c++) {
            if (tree->distribution.probs[c] > 0.01) {
                fprintf(stream, " [c=%zu: %.2f]", c, tree->distribution.probs[c]);
            }
        }
        fprintf(stream, "\n");
        return;
    }
    fprintf(stream, "SPLIT (samples: %zu, entropy: %.4f) ? ", tree->num_samples, tree->entropy);
    question_print(tree->question, stream);
    fprintf(stream, "\n");
    for (int i = 0; i < indent; i++) {
        fprintf(stream, "  ");
    }
    fprintf(stream, "  [TRUE]\n");
    decision_tree_print(tree->true_branch, indent + 2, stream);
    for (int i = 0; i < indent; i++) {
        fprintf(stream, "  ");
    }
    fprintf(stream, "  [FALSE]\n");
    decision_tree_print(tree->false_branch, indent + 2, stream);
}

void decision_tree_free(const DecisionTree* tree)
{
    if (!tree) {
        return;
    }
    if (tree->is_leaf) {
        distribution_free(tree->distribution);
    } else {
        decision_tree_free(tree->true_branch);
        decision_tree_free(tree->false_branch);
    }
    spatter_free((void*)tree);
}
