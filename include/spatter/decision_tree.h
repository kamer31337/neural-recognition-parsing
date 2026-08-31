#ifndef SPATTER_DECISION_TREE_H
#define SPATTER_DECISION_TREE_H

#include "spatter/common.h"
#include "spatter/feature.h"

typedef struct TrainingSample {
    Context context;
    int target_class;
    double weight;
} TrainingSample;

typedef struct Distribution {
    size_t num_classes;
    double* probs;
    double* log_probs;
} Distribution;

typedef struct DecisionTree {
    bool is_leaf;
    size_t num_samples;
    double entropy;
    Question question;
    const struct DecisionTree* true_branch;
    const struct DecisionTree* false_branch;
    Distribution distribution;
} DecisionTree;

typedef struct TreeTrainConfig {
    size_t max_depth;
    size_t min_samples_leaf;
    size_t min_samples_split;
    double min_info_gain;
    double smoothing_lambda;
    size_t num_classes;
} TreeTrainConfig;

TreeTrainConfig tree_train_config_default(size_t num_classes);
Distribution distribution_create(size_t num_classes);
Distribution distribution_copy(Distribution src);
void distribution_free(Distribution dist);
Distribution distribution_from_counts(const double* counts, size_t num_classes, double smoothing_alpha);
Distribution distribution_interpolate(Distribution leaf_dist, Distribution parent_dist, double lambda);
double calculate_entropy(const double* counts, size_t num_classes, double total_weight);

const DecisionTree* decision_tree_create_leaf(Distribution dist, size_t num_samples, double entropy);
const DecisionTree* decision_tree_create_split(Question q, const DecisionTree* true_br, const DecisionTree* false_br, size_t num_samples, double entropy);
const DecisionTree* decision_tree_train(const TrainingSample* samples, size_t num_samples, const Question* candidate_questions, size_t num_questions, TreeTrainConfig config, size_t current_depth, Distribution parent_dist);
Distribution decision_tree_predict(const DecisionTree* tree, const Context* ctx);
void decision_tree_print(const DecisionTree* tree, int indent, FILE* stream);
void decision_tree_free(const DecisionTree* tree);

#endif /* SPATTER_DECISION_TREE_H */
