# mdp

A small single-header Markdown parser written in C.

## Features

* Markdown lexer and parser
* Headings (`#`, `##`, etc.)
* Ordered and unordered lists
* Block quotes (`>`)
* Inline code and fenced code blocks
* Bold (`**text**` / `__text__`)
* Emphasis (`*text*` / `_text_`)
* Paragraph and newline handling
* AST generation for custom processing

## Installation

Copy `mdp.h` into your project.

In **one** source file, define `MDP_IMPLEMENTATION` before including the header:

```c
#define MDP_IMPLEMENTATION
#include "mdp.h"
```

In all other files:

```c
#include "mdp.h"
```

## Usage

### Lex Markdown

```c
MDP_Tokens tokens = mdp_lex(markdown_source);
```

### Parse Markdown

```c
MDP_Node *document = mdp_parse(&tokens);
```

### Debug Token Output

```c
mdp_dump_tokens(tokens);
```

### Debug AST Output

```c
mdp_dump_tree(document, 0, 2);
```

## Example

```c
const char *markdown =
    "# Hello World\n"
    "\n"
    "This is **bold** text.\n"
    "\n"
    "- Item 1\n"
    "- Item 2\n";

MDP_Tokens tokens = mdp_lex(markdown);
MDP_Node *doc = mdp_parse(&tokens);

mdp_dump_tree(doc, 0, 2);
```

### Try it out
```
$ gcc md2html.c -o md2html
$ ./md2html README.md index.html
```

## API

### Lexer

```c
MDP_Tokens mdp_lex(const char *char_stream);
```

Converts a Markdown string into a token stream.

### Parser

```c
MDP_Node *mdp_parse(MDP_Tokens *tokens);
```

Builds an abstract syntax tree (AST) from a token stream.

## License

MIT License.
