#include "spatter/parser.h"
#include <ctype.h>

ParserConfig parser_config_default(void)
{
    ParserConfig config = { .beam_width = 16, .max_steps = 32 };
    return config;
}

SpatterParser spatter_parser_create(SpatterModels models, ParserConfig config)
{
    SpatterParser parser = { .models = models, .config = config };
    return parser;
}

void spatter_parser_free(SpatterParser parser)
{
    spatter_models_free(parser.models);
}

const List* spatter_tokenize(StrView sentence)
{
    StrView trimmed = str_view_trim(sentence);
    if (str_view_is_empty(trimmed)) {
        return list_empty();
    }
    const List* words_rev = list_empty();
    size_t i = 0;
    while (i < trimmed.length) {
        while (i < trimmed.length && isspace((unsigned char)trimmed.data[i])) {
            i++;
        }
        if (i >= trimmed.length) {
            break;
        }
        size_t start = i;
        while (i < trimmed.length && !isspace((unsigned char)trimmed.data[i])) {
            i++;
        }
        StrView* token = (StrView*)spatter_alloc(sizeof(StrView));
        *token = str_view_create(trimmed.data + start, i - start);
        words_rev = list_cons(token, words_rev);
    }
    const List* words = list_reverse(words_rev);
    list_free_nodes(words_rev);
    return words;
}

Result spatter_parse(const SpatterParser* parser, StrView sentence)
{
    if (!parser) {
        return result_err("Parser instance is NULL");
    }
    const List* token_list = spatter_tokenize(sentence);
    size_t num_words = list_length(token_list);
    if (num_words == 0) {
        list_free_nodes(token_list);
        return result_err("Sentence has zero tokens");
    }

    StrView* words = (StrView*)spatter_alloc(num_words * sizeof(StrView));
    const List* curr_tok = token_list;
    size_t w_idx = 0;
    while (curr_tok) {
        const StrView* tok_ptr = (const StrView*)curr_tok->head;
        words[w_idx++] = *tok_ptr;
        spatter_free((void*)tok_ptr);
        curr_tok = curr_tok->tail;
    }
    list_free_nodes(token_list);

    const List* beam = beam_search_init(words, num_words, &parser->models, parser->config.beam_width);
    spatter_free(words);

    for (size_t step = 0; step < parser->config.max_steps; step++) {
        bool all_complete = true;
        const List* curr_st = beam;
        while (curr_st) {
            const ParseState* st = (const ParseState*)curr_st->head;
            if (st && !st->is_complete) {
                all_complete = false;
                break;
            }
            curr_st = curr_st->tail;
        }
        if (all_complete) {
            break;
        }
        const List* next_beam = beam_search_step(beam, &parser->models, parser->config.beam_width);
        if (next_beam == beam) {
            break;
        }
        const List* to_free = beam;
        beam = next_beam;
        const List* f_curr = to_free;
        while (f_curr) {
            parse_state_free((const ParseState*)f_curr->head);
            f_curr = f_curr->tail;
        }
        list_free_nodes(to_free);
    }

    const ParseNode* best_tree = beam_search_get_best_tree(beam);
    if (!best_tree) {
        const List* f_curr = beam;
        while (f_curr) {
            parse_state_free((const ParseState*)f_curr->head);
            f_curr = f_curr->tail;
        }
        list_free_nodes(beam);
        return result_err("Beam search failed to produce a valid parse tree");
    }

    const List* f_curr = beam;
    while (f_curr) {
        const ParseState* st = (const ParseState*)f_curr->head;
        if (st) {
            list_free_nodes(st->frontier);
            spatter_free((void*)st);
        }
        f_curr = f_curr->tail;
    }
    list_free_nodes(beam);

    return result_ok(best_tree);
}
