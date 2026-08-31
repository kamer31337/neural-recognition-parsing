#include "spatter/model.h"

SpatterModels spatter_models_create(const DecisionTree* pos, const DecisionTree* ext, const DecisionTree* lbl)
{
    SpatterModels models = { .pos_model = pos, .ext_model = ext, .lbl_model = lbl };
    return models;
}

void spatter_models_free(SpatterModels models)
{
    decision_tree_free(models.pos_model);
    decision_tree_free(models.ext_model);
    decision_tree_free(models.lbl_model);
}

Distribution spatter_predict_tag(const SpatterModels* models, const Context* ctx)
{
    if (!models || !models->pos_model) {
        Distribution empty = { .num_classes = 0, .probs = NULL, .log_probs = NULL };
        return empty;
    }
    return decision_tree_predict(models->pos_model, ctx);
}

Distribution spatter_predict_extension(const SpatterModels* models, const Context* ctx)
{
    if (!models || !models->ext_model) {
        Distribution empty = { .num_classes = 0, .probs = NULL, .log_probs = NULL };
        return empty;
    }
    return decision_tree_predict(models->ext_model, ctx);
}

Distribution spatter_predict_label(const SpatterModels* models, const Context* ctx)
{
    if (!models || !models->lbl_model) {
        Distribution empty = { .num_classes = 0, .probs = NULL, .log_probs = NULL };
        return empty;
    }
    return decision_tree_predict(models->lbl_model, ctx);
}

SpatterModels spatter_train_models(const TrainingSample* pos_samples, size_t num_pos, const TrainingSample* ext_samples, size_t num_ext, const TrainingSample* lbl_samples, size_t num_lbl, const Question* questions, size_t num_questions)
{
    Distribution null_dist = { .num_classes = 0, .probs = NULL, .log_probs = NULL };
    TreeTrainConfig pos_config = tree_train_config_default(TAG_COUNT);
    TreeTrainConfig ext_config = tree_train_config_default(EXT_COUNT);
    TreeTrainConfig lbl_config = tree_train_config_default(LBL_COUNT);

    const DecisionTree* pos_tree = decision_tree_train(pos_samples, num_pos, questions, num_questions, pos_config, 0, null_dist);
    const DecisionTree* ext_tree = decision_tree_train(ext_samples, num_ext, questions, num_questions, ext_config, 0, null_dist);
    const DecisionTree* lbl_tree = decision_tree_train(lbl_samples, num_lbl, questions, num_questions, lbl_config, 0, null_dist);

    return spatter_models_create(pos_tree, ext_tree, lbl_tree);
}
