#include "../src/mainDefs.h"
#include "../src/constantDefs.h"

#include "../tmp/lexerStruct.h"
#include "../src/parserStructDefs.h"

#define NUM_RULES 256

// parses the generated parse table into an includable .h file with the appropriate struct representation
int main() {
	// input file
	FILE *in;
	in = fopen("./tmp/parserTable.txt","r");
	if (in == NULL) { // if file open failed, return an error
		return -1;
	}
	// output files
	FILE *header;
	header = fopen("./tmp/parserStruct.h","w");
	if (header == NULL) { // if file open failed, return an error
		return -1;
	}
	FILE *pnr;
	pnr = fopen("./tmp/parserNodeRaw.h","w");
	if (pnr == NULL) { // if file open failed, return an error
		return -1;
	}
	FILE *rltt;
	rltt = fopen("./tmp/ruleLhsTokenTypeRaw.h","w");
	if (rltt == NULL) { // if file open failed, return an error
		return -1;
	}
	FILE *rlts;
	rlts = fopen("./tmp/ruleLhsTokenStringRaw.h","w");
	if (rlts == NULL) { // if file open failed, return an error
		return -1;
	}
	FILE *rrl;
	rrl = fopen("./tmp/ruleRhsLengthRaw.h","w");
	if (rrl == NULL) { // if file open failed, return an error
		return -1;
	}
	// print the necessary prologue into the .h
	fprintf(header, "#ifndef _PARSER_STRUCT_H_\n");
	fprintf(header, "#define _PARSER_STRUCT_H_\n\n");
	fprintf(header, "#include \"../tmp/lexerStruct.h\"\n");
	fprintf(header, "#include \"../src/parserStructDefs.h\"\n\n");
	fprintf(header, "#define NUM_RULES %d\n\n", NUM_RULES);

	// now, process the input file
	char lineBuf[MAX_STRING_LENGTH]; // line data buffer

	// first, scan ahead to the parser table
	bool tableFound = false;
	for(;;) {
		char *retVal = fgets(lineBuf, MAX_STRING_LENGTH, in);
		if (retVal == NULL) { // if we've reached the end of the file, break out of the loop
			break;
		}
		if (strcmp(lineBuf,"--Parsing Table--\n") == 0) { // if we've found the beginning of the parse table, break out of the loop
			tableFound = true;
			break;
		}
	}
	// if we couldn't find a parse table block, return with an error
	if (!tableFound) {
		return -1;
	}

	// now, extract the token ordering from the header row
	// read the header into the line buffer
	char *retVal = fgets(lineBuf, MAX_STRING_LENGTH, in);
	if (retVal == NULL) { // if we've reached the end of the file, return an error code
		return -1;
	}
	// discard the initial "State"
	char *lbCur = lineBuf;
	char junk[MAX_STRING_LENGTH];
	sscanf(lbCur, "%s", junk);
	lbCur += strlen(junk)+1; // scan past the "State"
	// read in the token ordering itself
	bool parsingNonTerms = false; // whether we have reached the nonterminal part yet
	unsigned int nonTermCount = 0;
	string token; // temporary token string
	map<unsigned int, string> tokenOrder;
	map<string, unsigned int> tokenClassifier;
	for(;;) {
		if (sscanf(lbCur, "%s", junk) < 1) {
			break;
		}
		lbCur += strlen(junk)+1;
		// allocate a string with this token in it
		token = "TOKEN_";
		// $end token special-casing
		if (strcmp(junk, "$end") == 0) {
			token += "END";
		} else {
			token += junk;
		}
		if (token == "TOKEN_Program") {
			parsingNonTerms = true;
		}
		if (parsingNonTerms) {
			fprintf(header, "#define %s NUM_TOKENS + %d\n", token.c_str(), nonTermCount);
			nonTermCount++;
		}
		// push the string to the token ordering map
		tokenOrder.insert( make_pair(tokenOrder.size(), token) );
		unsigned int mapping = string2TokenType(token);
		if (mapping != NUM_TOKENS) { // if it's a lexerStruct mapping
			tokenClassifier.insert( make_pair(token, mapping) );
		} else { // otherwise, if it's a parserStruct mapping
			tokenClassifier.insert( make_pair(token, (NUM_TOKENS + (nonTermCount - 1))) );
		}
	}
	// print out the definition of the number of nonterminals
	fprintf(header, "\n");
	fprintf(header, "#define NUM_NONTERMS %d\n\n", nonTermCount);
	// print out the definition for the total label count
	fprintf(header, "#define NUM_LABELS NUM_TOKENS + NUM_NONTERMS\n\n");
	// print out the epilogue into the .h
	fprintf(header, "void parserInit( int ruleLhsTokenType[NUM_RULES],\n");
	fprintf(header, "\t\tconst char *ruleLhsTokenString[NUM_RULES] );\n\n");
	fprintf(header, "#endif\n");

	// now, back up in the file and scan ahead to the rule declarations
	fseek(in, 0, SEEK_SET);
	bool rulesFound = false;
	for(;;) {
		char *retVal = fgets(lineBuf, MAX_STRING_LENGTH, in);
		if (retVal == NULL) { // if we've reached the end of the file, break out of the loop
			break;
		}
		if (strcmp(lineBuf,"Rules: \n") == 0) { // if we've found the beginning of the rules, break out of the loop
			rulesFound = true;
			break;
		}
	}
	// if we couldn't find a rule block, return with an error
	if (!rulesFound) {
		return -1;
	}

	// print out the ruleLhsTokenType, ruleLshTokenString, and ruleRhsLength array initializers
	fprintf(rltt, "const int ruleLhsTokenType[NUM_RULES] = {\n");
	fprintf(rlts, "const char *ruleLhsTokenString[NUM_RULES] = {\n");
	fprintf(rrl, "const unsigned int ruleRhsLength[NUM_RULES] = {\n");
	// get rule lengths
	for (unsigned int i=0; true; i++) { // per-rule line loop
		// read in a line
		char *retVal = fgets(lineBuf, MAX_STRING_LENGTH, in);
		if (retVal == NULL) { // if we've reached the end of the file, break out of the loop
			return -1;
		}
		char *lbCur = lineBuf;
		// discard the junk before the rule's lhs
		char junk[MAX_STRING_LENGTH];
		sscanf(lbCur, "%s", junk); // scan the first token of the line
		if (junk[0] == 'N') { // break if we've reached the end of the rule set
			break;
		}
		// scan over to the next token
		lbCur += strlen(junk);
		while (lbCur[0] == ' ' || lbCur[0] == 't') {
			lbCur++;
		}
		// scan in the lhs of the rule
		sscanf(lbCur, "%s", junk);
		string lhs(junk); // log the lhs in a string wrapper
		// scan over to the next token
		lbCur += strlen(junk);
		while (lbCur[0] == ' ' || lbCur[0] == 't') {
			lbCur++;
		}
		sscanf(lbCur, "%s", junk); // scan and throw away the next "->" token in the line
		// scan over to the next token
		lbCur += strlen(junk);
		while (lbCur[0] == ' ' || lbCur[0] == 't') {
			lbCur++;
		}
		// now, count the number of elements on the RHS
		int rhsElements = 0;
		for(;;) {
			if (sscanf(lbCur, "%s", junk) < 1 || junk[0] == '(') { // break wif we reach the end of the line
				break;
			}
			// scan over to the next token
			lbCur += strlen(junk);
			while (lbCur[0] == ' ' || lbCur[0] == 't') {
				lbCur++;
			}
			rhsElements++;
		}
		// then, log the lhs tokenType, lhs tokenString and rhs size of the rule in the corresponding arrays
		if (lhs != "$accept") {
			fprintf(rltt, "\tTOKEN_%s,\n", lhs.c_str());
			fprintf(rlts, "\t\"%s\",\n", lhs.c_str());
		} else {
			fprintf(rltt, "\t0,\n");
			fprintf(rlts, "\t\"\",\n");
		}
		fprintf(rrl, "\t%d,\n", rhsElements);
	}
	fprintf(rltt, "};\n");
	fprintf(rlts, "};\n");
	fprintf(rrl, "};\n");

	// now, scan ahead to the parse table
	tableFound = false;
	for(;;) {
		char *retVal = fgets(lineBuf, MAX_STRING_LENGTH, in);
		if (retVal == NULL) { // if we've reached the end of the file, break out of the loop
			break;
		}
		if (strcmp(lineBuf,"--Parsing Table--\n") == 0) { // if we've found the beginning of the parse table, break out of the loop
			tableFound = true;
			break;
		}
	}
	if (!tableFound) { // if we couldn't find a parse table block, return with an error
		return -1;
	} else { // otherwise, eat the header line and return an error if this fails
		char *retVal = fgets(lineBuf, MAX_STRING_LENGTH, in);
		if (retVal == NULL) { // if we've reached the end of the file, break out of the loop
			return -1;
		}
	}

	// finally, process the raw parse table actions
	ParserNode parserNode[NUM_RULES][NUM_TOKENS + nonTermCount];
	for (unsigned int i=0; i < NUM_RULES; i++) {
		for (unsigned int j=0; j < (NUM_TOKENS + nonTermCount); j++) {
			parserNode[i][j].action = ACTION_ERROR;
			parserNode[i][j].n = ACTION_ERROR;
		}
	}
	for(;;) {
		char *retVal = fgets(lineBuf, MAX_STRING_LENGTH, in);
		if (retVal == NULL || lineBuf[0] == 'N') { // if we've reached the end of the table, break out of the loop
			break;
		}
		// the line was valid, so now try parsing the data out of it
		int fromState;
		// first, read the state number
		char *lbCur = lineBuf;
		sscanf(lbCur, "%s", junk);
		// advance past the current token
		lbCur += strlen(junk);
		while(lbCur[0] == ' ' || lbCur[0] == '\t') {
			lbCur++;
		}
		// parse out the state number from the string
		fromState = atoi(junk);
		// now, read all of the transitions for this state
		for(unsigned int i=0; i<tokenOrder.size(); i++) {
			sscanf(lbCur, "%s", junk); // read a transition
			// advance past the current token
			lbCur += strlen(junk);
			while(lbCur[0] == ' ' || lbCur[0] == '\t') {
				lbCur++;
			}
			// branch based on the type of transition action it is
			if (junk[0] == 's') { // shift action
				parserNode[fromState][tokenClassifier[tokenOrder[i]]] = (ParserNode){ ACTION_SHIFT, atoi(junk+1) };
			} else if (junk[0] == 'r') { // reduce action
				parserNode[fromState][tokenClassifier[tokenOrder[i]]] = (ParserNode){ ACTION_REDUCE, atoi(junk+1) };
			} else if (junk[0] == 'a') { // accept action
				parserNode[fromState][tokenClassifier[tokenOrder[i]]] = (ParserNode){ ACTION_ACCEPT, ACTION_ERROR };
			} else if (junk[0] == 'g') { // goto action
				parserNode[fromState][tokenClassifier[tokenOrder[i]]] = (ParserNode){ ACTION_GOTO, atoi(junk+1) };
			}
			// do nothing on error states; all states have been defaulted to ACTION_ERROR already
		}
	}

	// print out the parserNode array initializer
	fprintf(pnr, "const ParserNode parserNode[NUM_RULES][NUM_LABELS] = {\n");
	// per-rule loop
	for (unsigned int i=0; i < NUM_RULES; i++) {
		fprintf(pnr, "\t{\n");
		// per-label loop
		for (unsigned int j=0; j < (NUM_TOKENS + nonTermCount); j++) {
			string actionString = (
				parserNode[i][j].action == ACTION_SHIFT ? "ACTION_SHIFT" :
				parserNode[i][j].action == ACTION_REDUCE ? "ACTION_REDUCE" :
				parserNode[i][j].action == ACTION_ACCEPT ? "ACTION_ACCEPT" :
				parserNode[i][j].action == ACTION_GOTO ? "ACTION_GOTO" :
				parserNode[i][j].action == ACTION_ERROR ? "ACTION_ERROR" :
				""
			);
			fprintf(pnr, "\t\t{ %s, %u }, /* [%u][%u] */\n", actionString.c_str(), parserNode[i][j].n, i, j);
		}
		fprintf(pnr, "\t},\n");
	}
	fprintf(pnr, "};\n");

	// finally, return normally
	return 0;
}
