#ifndef SPATTER_MODEL_H
#define SPATTER_MODEL_H

#include "spatter/common.h"
#include "spatter/decision_tree.h"
#include "spatter/tree.h"

typedef struct SpatterModels {
    const DecisionTree* pos_model;
    const DecisionTree* ext_model;
    const DecisionTree* lbl_model;
} SpatterModels;

SpatterModels spatter_models_create(const DecisionTree* pos, const DecisionTree* ext, const DecisionTree* lbl);
void spatter_models_free(SpatterModels models);
Distribution spatter_predict_tag(const SpatterModels* models, const Context* ctx);
Distribution spatter_predict_extension(const SpatterModels* models, const Context* ctx);
Distribution spatter_predict_label(const SpatterModels* models, const Context* ctx);
SpatterModels spatter_train_models(const TrainingSample* pos_samples, size_t num_pos, const TrainingSample* ext_samples, size_t num_ext, const TrainingSample* lbl_samples, size_t num_lbl, const Question* questions, size_t num_questions);

#endif /* SPATTER_MODEL_H */
