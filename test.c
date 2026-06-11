#define MDP_IMPLEMENTATION
#include "mdp.h"

int main(void) {
	MDP_Tokens toks = mdp_lex(
		"# Text\n"
		"How are you?\n"
		"Buddy!\n"
		" - Fine\n"
		" - Good\n"
		" - Well\n"
		"Cool software:\n"
		"1. GCC\n"
		"2. Linux\n"
		"0. m7c\n"
		"\n"
		"3 _just_ `in` **case**\n"
		"## Just text\n"
		"WoWoWoW\n"
		"# Yet another text\n"
		"Some text\n"
		"> Some quote\n"
		"> LOLOLOLOL\n"
	);

	mdp_dump_tokens(toks);
	MDP_Node *doc = mdp_parse(&toks);
	mdp_dump_tree(doc, 0, 4);
	return 0;
}
