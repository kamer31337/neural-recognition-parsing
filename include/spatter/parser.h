#ifndef SPATTER_PARSER_H
#define SPATTER_PARSER_H

#include "spatter/common.h"
#include "spatter/result.h"
#include "spatter/str_view.h"
#include "spatter/tree.h"
#include "spatter/model.h"
#include "spatter/beam_search.h"
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct ParserConfig {
    size_t beam_width;
    size_t max_steps;
} ParserConfig;

typedef struct SpatterParser {
    SpatterModels models;
    ParserConfig config;
} SpatterParser;

ParserConfig parser_config_default(void);
SpatterParser spatter_parser_create(SpatterModels models, ParserConfig config);
void spatter_parser_free(SpatterParser parser);
const List* spatter_tokenize(StrView sentence);
Result spatter_parse(const SpatterParser* parser, StrView sentence);

#endif /* SPATTER_PARSER_H */
