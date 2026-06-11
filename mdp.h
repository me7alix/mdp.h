#ifndef MDP_H_
#define MDP_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <ctype.h>

typedef enum {
	MDP_TOK_HEADING,
	MDP_TOK_ORD_LIST,
	MDP_TOK_UNORD_LIST,
	MDP_TOK_QUOTE,
	MDP_TOK_2NL,
	_MDP_TOK_INLINE_START,
	MDP_TOK_STRONG,
	MDP_TOK_EMPHASIS,
	MDP_TOK_INLINE_CODE,
	MDP_TOK_CHAR,
	MDP_TOK_NL,
	_MDP_TOK_INLINE_END,
	MDP_TOK_EOF,
} MDP_TokenKind;

typedef struct {
	MDP_TokenKind kind;
	unsigned data;
} MDP_Token;

typedef struct {
	MDP_Token *items;
	size_t count;
	size_t capacity;
} MDP_Tokens;

typedef struct {
	MDP_Tokens *tokens;
	size_t count;
} MDP_Parser;

typedef enum {
/* Container */
	MDP_NODE_DOC,
	MDP_NODE_HEADING,
	MDP_NODE_PARAGRAPH,
	MDP_NODE_ORD_LIST,
	MDP_NODE_UNORD_LIST,
	MDP_NODE_QUOTE,

/* Inline */
	MDP_NODE_STRONG,
	MDP_NODE_EMPHASIS,
	MDP_NODE_INLINE_CODE,
	MDP_NODE_TEXT,
	MDP_NODE_NL,

/* Helper */
	MDP_NODE_ORD_LIST_ITEM,
} MDP_NodeKind;

typedef struct MDP_Node MDP_Node;
struct MDP_Node {
	MDP_NodeKind kind;
	MDP_Node *body;
	MDP_Node *next;

	union {
		struct {
			unsigned level;
			MDP_Node *title;
		} heading;
		unsigned idx;
		const char *text;
	} as;
};

#define mdp_node_foreach(vn, list) \
	for (MDP_Node *vn = list; vn; vn = vn->next)

MDP_Tokens mdp_lex(const char *char_stream);
MDP_Node *mdp_parse(MDP_Tokens *toks);

#endif // MDP_H_

#ifdef MDP_IMPLEMENTATION

#define _mdp_da_append(da, item)                               \
	do {                                                       \
		if ((da)->capacity == 0) (da)->capacity = 256;         \
		if ((da)->count >= (da)->capacity - 1 || !(da)->items) \
			(da)->items = realloc((da)->items, sizeof(*(da)->items) * ((da)->capacity *= 2)); \
		(da)->items[(da)->count++] = (item);                   \
	} while (0)

#define mdp_da_free(da)         \
	do {                        \
		if ((da)->items) {      \
			free((da)->items);  \
			(da)->items = NULL; \
			(da)->count = 0;    \
			(da)->capacity = 0; \
		}                       \
	} while (0)

/* Lexer stage */

void _mdp_tokens_append(MDP_Tokens *toks, MDP_TokenKind kind, char data) {
	_mdp_da_append(toks, ((MDP_Token){kind, data}));
}

MDP_Tokens mdp_lex(const char *char_stream) {
	MDP_Tokens toks = {0};
	size_t count = 0;
	goto start;

#define append _mdp_tokens_append
#define CHN() char_stream[count++]
#define CHP() char_stream[count]

	while (true) {
		switch (CHP()) {
		case '\0':
			goto finish;

		case '`':
			append(&toks, MDP_TOK_INLINE_CODE, CHN());
			break;

		case '_':
			append(&toks, MDP_TOK_EMPHASIS, CHN());
			break;

		case '*':
			CHN();
			if (CHP() == '*') {
				append(&toks, MDP_TOK_STRONG, CHN());
			} else {
				append(&toks, MDP_TOK_CHAR, '*');
			} break;

		case '\n':
			CHN();
			if (CHP() == '\n') append(&toks, MDP_TOK_2NL, CHN());
			else append(&toks, MDP_TOK_NL, '\n');
		start:
			while (CHP() == ' ') CHN();

			switch (CHP()) {
			case '\0':
				goto finish;

			case '#':
				CHN();
				if (CHN() == '#') {
					if (CHN() == '#') {
						append(&toks, MDP_TOK_HEADING, 3);
					} else append(&toks, MDP_TOK_HEADING, 2);
				} else append(&toks, MDP_TOK_HEADING, 1);
				while (CHP() == ' ') CHN();
				break;

			case '>':
				append(&toks, MDP_TOK_QUOTE, CHN());
				while (CHP() == ' ') CHN();
				break;

			case '-':
				append(&toks, MDP_TOK_UNORD_LIST, CHN());
				while (CHP() == ' ') CHN();
				break;

			default:
				if (isdigit(CHP())) {
					char str[16] = {0};
					size_t cnt = 0;
					while (isdigit(CHP()))
						str[cnt++] = CHN();

					if (CHP() == '.') {
						int idx = atoi(str);
						append(&toks, MDP_TOK_ORD_LIST, idx);
						CHN(); while (CHP() == ' ') CHN();
					} else {
						for (size_t i = 0; i < cnt; i++) {
							append(&toks, MDP_TOK_CHAR, str[i]);
						}
					}
				}
			}
			break;

		default:
			append(&toks, MDP_TOK_CHAR, CHN());
		}
	}

finish:
	append(&toks, MDP_TOK_EOF, 0);

#undef CHN
#undef CHP
#undef append

	return toks;
}

void mdp_dump_tokens(MDP_Tokens toks) {
	for (size_t i = 0; i < toks.count; i++) {
		switch (toks.items[i].kind) {
		case MDP_TOK_STRONG:      printf("STRONG\n");      break;
		case MDP_TOK_NL:          printf("NL\n");          break;
		case MDP_TOK_2NL:         printf("2NL\n");         break;
		case MDP_TOK_QUOTE:       printf("QUOTE\n");       break;
		case MDP_TOK_INLINE_CODE: printf("INLINE_CODE\n"); break;
		case MDP_TOK_EMPHASIS:    printf("EMPHASIS\n");    break;
		case MDP_TOK_UNORD_LIST:  printf("UNORD_LIST\n");  break;
		case MDP_TOK_EOF:         printf("EOF\n");         break;
		case MDP_TOK_HEADING:
			printf("HEADING(%u)\n", toks.items[i].data);
			break;
		case MDP_TOK_ORD_LIST:
			printf("ORD_LIST(%u)\n", toks.items[i].data);
			break;
		case MDP_TOK_CHAR:
			char ch = toks.items[i].data;
			if (ch == '\n') printf("CHAR(NL)\n");
			else printf("CHAR(%c)\n", ch);
		}
	}
}

/* Parser stage */

#define next(p) (p)->tokens->items[(p)->count++]
#define peek(p) (p)->tokens->items[(p)->count]
#define peek2(p) (p)->tokens->items[(p)->count+1]

#define node(kind, ...) _mdp_node(kind, &(MDP_Node){__VA_ARGS__})
MDP_Node *_mdp_node(MDP_NodeKind kind, MDP_Node *node) {
	MDP_Node *n = malloc(sizeof(MDP_Node));
	*n = *node;
	n->kind = kind;
	return n;
}

void _mdp_node_append(MDP_Node **head, MDP_Node *n) {
	if (!*head) {
		*head = n;
		return;
	}

	MDP_Node *cur = *head;
	while (cur->next)
		cur = cur->next;
	cur->next = n;
}

bool _mdp_is_tok_inline(MDP_Token tok) {
	return tok.kind > _MDP_TOK_INLINE_START &&
	       tok.kind < _MDP_TOK_INLINE_END;
}

MDP_Node *_mdp_parse(MDP_Parser *p, bool is_inline);

MDP_Node *_mdp_parse_paragraph(MDP_Parser *p) {
	MDP_Node *n = node(MDP_NODE_PARAGRAPH, 0);
	while (_mdp_is_tok_inline(peek(p)))
		_mdp_node_append(&n->body, _mdp_parse(p, true));
	if (peek(p).kind == MDP_TOK_2NL) {
		_mdp_node_append(&n->body, node(MDP_NODE_NL, 0));
		next(p);
	}
	return n;
}

MDP_Node *_mdp_parse_text(MDP_Parser *p) {
	struct {
		char *items;
		size_t count;
		size_t capacity;
	} sb = {0};
	while (peek(p).kind == MDP_TOK_CHAR)
		_mdp_da_append(&sb, (char)next(p).data);
	_mdp_da_append(&sb, '\0');
	return node(MDP_NODE_TEXT, .as.text = sb.items);
}

MDP_Node *_mdp_parse(MDP_Parser *p, bool is_inline) {
	if (!is_inline) {
		switch (peek(p).kind) {
		case MDP_TOK_HEADING: {
			unsigned level = next(p).data;
			MDP_Node *n = node(
				MDP_NODE_HEADING,
				.as.heading.level = level);
			while (_mdp_is_tok_inline(peek(p))) {
				MDP_Node *c = _mdp_parse(p, true);
				if (c->kind == MDP_NODE_NL) break;
				_mdp_node_append(&n->as.heading.title, c);
			}
			while (
				!(peek(p).kind == MDP_TOK_HEADING &&
				peek(p).data <= level) &&
				peek(p).kind != MDP_TOK_EOF
			) {
				MDP_Node *c = _mdp_parse(p, false);
				_mdp_node_append(&n->body, c);
			}
			return n;
		}

		case MDP_TOK_QUOTE: {
			MDP_Node *n = node(MDP_NODE_QUOTE, 0);
			while (peek(p).kind == MDP_TOK_QUOTE) {
				next(p);
				while (_mdp_is_tok_inline(peek(p))) {
					MDP_Node *c = _mdp_parse(p, true);
					_mdp_node_append(&n->body, c);
				}
			}
			return n;
		}

		case MDP_TOK_UNORD_LIST: {
			MDP_Node *n = node(MDP_NODE_UNORD_LIST, 0);
			while (peek(p).kind == MDP_TOK_UNORD_LIST) {
				next(p);
				MDP_Node *c = _mdp_parse(p, false);
				_mdp_node_append(&n->body, c);
			}
			return n;
		}

		case MDP_TOK_ORD_LIST: {
			MDP_Node *n = node(MDP_NODE_ORD_LIST, 0);
			while (peek(p).kind == MDP_TOK_ORD_LIST) {
				unsigned idx = next(p).data;
				_mdp_node_append(
					&n->body,
					node(MDP_NODE_ORD_LIST_ITEM,
						.as.idx = idx,
						.body = _mdp_parse(p, false)
					)
				);
			}
			return n;
		}

		default:
			return _mdp_parse_paragraph(p);
		}
	} else {
		switch (peek(p).kind) {
		case MDP_TOK_STRONG:
		case MDP_TOK_EMPHASIS:
		case MDP_TOK_INLINE_CODE:
			MDP_TokenKind kind = peek(p).kind;
			MDP_NodeKind nk =
				kind == MDP_TOK_STRONG      ? MDP_NODE_STRONG      :
				kind == MDP_TOK_EMPHASIS    ? MDP_NODE_EMPHASIS    :
				kind == MDP_TOK_INLINE_CODE ? MDP_NODE_INLINE_CODE : 0;
			MDP_Node *n = node(nk, 0);
			next(p); while (
				peek(p).kind != kind &&
				peek(p).kind != MDP_TOK_EOF
			) {
				MDP_Node *c = _mdp_parse(p, true);
				_mdp_node_append(&n->body, c);
			}
			next(p);
			return n;
		case MDP_TOK_NL:
			next(p);
			return node(MDP_NODE_NL, 0);
		case MDP_TOK_CHAR:
			return _mdp_parse_text(p);
		}
	}

	printf("%d %d\n", peek(p).kind, peek(p).data);
	assert(0);
}

MDP_Node *mdp_parse(MDP_Tokens *toks) {
	MDP_Parser p = {.tokens = toks};
	MDP_Node *doc = node(MDP_NODE_DOC, 0);
	while (peek(&p).kind != MDP_TOK_EOF)
		_mdp_node_append(&doc->body, _mdp_parse(&p, false));
	return doc;
}

void mdp_dump_tree(MDP_Node *n, int intend, int level) {
	printf("%*s", intend, "");
	switch (n->kind) {
	case MDP_NODE_DOC:
		printf("Document:\n");
		mdp_node_foreach (c, n->body)
			mdp_dump_tree(c, intend + level, level);
		break;
	case MDP_NODE_STRONG:
		printf("Strong:\n");
		mdp_node_foreach (c, n->body)
			mdp_dump_tree(c, intend + level, level);
		break;
	case MDP_NODE_EMPHASIS:
		printf("Emphasis:\n");
		mdp_node_foreach (c, n->body)
			mdp_dump_tree(c, intend + level, level);
		break;
	case MDP_NODE_INLINE_CODE:
		printf("InlineCode:\n");
		mdp_node_foreach (c, n->body)
			mdp_dump_tree(c, intend + level, level);
		break;
	case MDP_NODE_PARAGRAPH:
		printf("Paragraph:\n");
		mdp_node_foreach (c, n->body)
			mdp_dump_tree(c, intend + level, level);
		break;
	case MDP_NODE_UNORD_LIST:
		printf("UnordList:\n");
		mdp_node_foreach (c, n->body)
			mdp_dump_tree(c, intend + level, level);
		break;
	case MDP_NODE_QUOTE:
		printf("Quote:\n");
		mdp_node_foreach (c, n->body)
			mdp_dump_tree(c, intend + level, level);
		break;
	case MDP_NODE_ORD_LIST:
		printf("OrdList:\n");
		mdp_node_foreach (c, n->body)
			mdp_dump_tree(c, intend + level, level);
		break;
	case MDP_NODE_ORD_LIST_ITEM:
		printf("OrdListItem(%u):\n", n->as.idx);
		mdp_dump_tree(n->body, intend + level, level);
		break;
	case MDP_NODE_HEADING:
		printf("Heading(%u):\n", n->as.heading.level);
		printf("%*sTitle:\n", intend + level, "");
		mdp_node_foreach (c, n->as.heading.title)
			mdp_dump_tree(c, intend + level * 2, level);
		printf("%*sBody:\n", intend + level, "");
		mdp_node_foreach (c, n->body)
			mdp_dump_tree(c, intend + level * 2, level);
		break;
	case MDP_NODE_NL:
		printf("NewLine\n");
		break;
	case MDP_NODE_TEXT:
		printf("Text: %s\n", n->as.text);
	}
}

#undef next
#undef peek
#undef peek2
#undef node

#endif // MDP_IMPLEMENTATION
