#include "spatter/common.h"
#include "spatter/treebank.h"
#include "spatter/parser.h"
#include "spatter/eval.h"

static void print_banner(void)
{
    printf("======================================================================\n");
    printf("  SPATTER: Natural Language Parsing as Statistical Pattern Recognition\n");
    printf("  Reference: David M. Magerman (arXiv:cmp-lg/9405009 / ACL 1995)\n");
    printf("  Paradigm:  Pure Functional ISO C11 Implementation\n");
    printf("======================================================================\n\n");
}

int main(int argc, char** argv)
{
    print_banner();
    const char* train_path = (argc > 1) ? argv[1] : "data/sample_train.penn";
    const char* test_path = (argc > 2) ? argv[2] : "data/sample_test.penn";

    printf("[1] Loading training treebank from: %s\n", train_path);
    const List* train_trees = treebank_load_file(train_path);
    size_t num_train = list_length(train_trees);
    printf("    -> Successfully loaded %zu annotated sentences.\n\n", num_train);

    if (num_train == 0) {
        fprintf(stderr, "[ERROR] No training data found. Exiting.\n");
        return EXIT_FAILURE;
    }

    printf("[2] Extracting training instances (POS, Extensions, Labels, Questions)...\n");
    TreebankCorpus corpus = treebank_extract_corpus(train_trees);
    printf("    -> POS samples:       %zu\n", corpus.num_pos);
    printf("    -> Extension samples: %zu\n", corpus.num_ext);
    printf("    -> Label samples:     %zu\n", corpus.num_lbl);
    printf("    -> Questions pool:    %zu\n\n", corpus.num_questions);

    printf("[3] Training statistical decision trees via Information Gain (Entropy Reduction)...\n");
    SpatterModels models = spatter_train_models(corpus.pos_samples, corpus.num_pos, corpus.ext_samples, corpus.num_ext, corpus.lbl_samples, corpus.num_lbl, corpus.questions, corpus.num_questions);
    printf("    -> Decision tree models successfully trained.\n\n");

    ParserConfig config = parser_config_default();
    config.beam_width = 16;
    SpatterParser parser = spatter_parser_create(models, config);

    printf("[4] Demonstrating sentence parsing with Beam Search (BeamWidth=%zu)...\n", config.beam_width);
    const char* demo_sentences[] = {
        "The dog chased the cat",
        "A boy saw the dog",
        "She ate the good food"
    };
    size_t num_demos = sizeof(demo_sentences) / sizeof(demo_sentences[0]);

    for (size_t i = 0; i < num_demos; i++) {
        StrView sent = str_view_from_cstr(demo_sentences[i]);
        printf("----------------------------------------------------------------------\n");
        printf("Input Sentence: \"%s\"\n", demo_sentences[i]);
        Result res = spatter_parse(&parser, sent);
        if (result_is_ok(res)) {
            const ParseNode* parse_tree = (const ParseNode*)res.as.ok_val;
            printf("Bracketed Parse: ");
            parse_node_to_bracketed(parse_tree, stdout);
            printf("\n\nHierarchical Tree:\n");
            parse_node_print_tree(parse_tree, 1, stdout);
            printf("\n");
            parse_node_free(parse_tree);
        } else {
            printf("Parsing Error: %s\n", result_unwrap_err(res));
        }
    }

    printf("======================================================================\n");
    printf("[5] Evaluating on test treebank: %s\n", test_path);
    const List* test_trees = treebank_load_file(test_path);
    size_t num_test = list_length(test_trees);
    printf("    -> Loaded %zu test sentences.\n", num_test);

    if (num_test > 0) {
        size_t total_gold_brackets = 0;
        size_t total_pred_brackets = 0;
        size_t total_matched_brackets = 0;
        size_t exact_matches = 0;

        const List* curr = test_trees;
        size_t test_idx = 1;
        while (curr) {
            const ParseNode* gold_tree = (const ParseNode*)curr->head;
            char* gold_buf = (char*)spatter_alloc(1024);
            FILE* mem_f = fopen("gold_tmp.txt", "w");
            if (mem_f) {
                parse_node_to_bracketed(gold_tree, mem_f);
                fclose(mem_f);
            }
            spatter_free(gold_buf);

            StrView* words = (StrView*)spatter_alloc(128 * sizeof(StrView));
            size_t w_count = 0;
            const List* c_curr = gold_tree->children;
            while (c_curr) {
                const ParseNode* cn = (const ParseNode*)c_curr->head;
                if (cn && cn->is_leaf) {
                    words[w_count++] = cn->word;
                } else if (cn && cn->children) {
                    const List* cc = cn->children;
                    while (cc) {
                        const ParseNode* ccn = (const ParseNode*)cc->head;
                        if (ccn && ccn->is_leaf) {
                            words[w_count++] = ccn->word;
                        }
                        cc = cc->tail;
                    }
                }
                c_curr = c_curr->tail;
            }

            char raw_sent[512] = { 0 };
            for (size_t w = 0; w < w_count; w++) {
                strncat(raw_sent, words[w].data, words[w].length);
                if (w + 1 < w_count) {
                    strcat(raw_sent, " ");
                }
            }
            spatter_free(words);

            if (strlen(raw_sent) > 0) {
                Result r = spatter_parse(&parser, str_view_from_cstr(raw_sent));
                if (result_is_ok(r)) {
                    const ParseNode* pred_tree = (const ParseNode*)r.as.ok_val;
                    ParsevalMetrics m = eval_compare_trees(gold_tree, pred_tree);
                    total_gold_brackets += m.gold_brackets;
                    total_pred_brackets += m.pred_brackets;
                    total_matched_brackets += m.matched_brackets;
                    if (m.exact_match) {
                        exact_matches++;
                    }
                    printf("    [Test %zu] \"%s\"\n      ", test_idx, raw_sent);
                    eval_metrics_print(m, stdout);
                    parse_node_free(pred_tree);
                }
            }
            test_idx++;
            curr = curr->tail;
        }

        double overall_prec = (total_pred_brackets > 0) ? ((double)total_matched_brackets / (double)total_pred_brackets) : 1.0;
        double overall_rec = (total_gold_brackets > 0) ? ((double)total_matched_brackets / (double)total_gold_brackets) : 1.0;
        double overall_f1 = (overall_prec + overall_rec > SPATTER_EPSILON) ? (2.0 * overall_prec * overall_rec / (overall_prec + overall_rec)) : 0.0;

        printf("\n----------------------------------------------------------------------\n");
        printf("OVERALL PARSEVAL RESULTS:\n");
        printf("  Precision:   %.2f%%\n", overall_prec * 100.0);
        printf("  Recall:      %.2f%%\n", overall_rec * 100.0);
        printf("  F1-Score:    %.2f%%\n", overall_f1 * 100.0);
        printf("  Exact Match: %zu / %zu (%.2f%%)\n", exact_matches, num_test, (double)exact_matches * 100.0 / (double)num_test);
    }

    remove("gold_tmp.txt");
    treebank_corpus_free(corpus);
    const List* t_curr = train_trees;
    while (t_curr) {
        parse_node_free((const ParseNode*)t_curr->head);
        t_curr = t_curr->tail;
    }
    list_free_nodes(train_trees);

    const List* te_curr = test_trees;
    while (te_curr) {
        parse_node_free((const ParseNode*)te_curr->head);
        te_curr = te_curr->tail;
    }
    list_free_nodes(test_trees);

    spatter_parser_free(parser);
    printf("\n[SPATTER] Execution completed successfully.\n");
    return EXIT_SUCCESS;
}
