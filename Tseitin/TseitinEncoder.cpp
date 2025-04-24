#include <iostream>
#include <vector>
#include <map>
#include <sstream>
using namespace std;

// operations are AND, OR, NOT
using Clause = vector<int>;
using Formula = vector<Clause>; // decided DIMACS is better

enum class TokenType {
    NOT_t, AND_t, OR_t, LP_t, RP_t, VAR_t, EOF_t
};

struct Token {
    TokenType type;
    string value;
};

vector<Token> tokenize(const string& formula) {
    vector<Token> tokens;
    int pos = 0;
    
    while (pos < formula.size()) {
        while (pos < formula.size() && isspace(formula[pos])) pos++;

        if (pos >= formula.size()) {
            tokens.push_back({TokenType::EOF_t, ""});
            break;
        }

        char c = formula[pos];
        if (c == '(') {
            tokens.push_back({TokenType::LP_t, "("});
            pos++;
        } else if (c == ')') {
            tokens.push_back({TokenType::RP_t, ")"});
            pos++;
        } else if (isalpha(c)) {
            int start = pos;
            while (pos < formula.size() && (isalnum(formula[pos]) || formula[pos] == '_')) pos++;
            string word = formula.substr(start, pos - start);

            if (word == "NOT") tokens.push_back({TokenType::NOT_t, word});
            else if (word == "AND") tokens.push_back({TokenType::AND_t, word});
            else if (word == "OR") tokens.push_back({TokenType::OR_t, word});
            else tokens.push_back({TokenType::VAR_t, word});
        } else {
            cout << "Unexpected character: " << c << endl;
            exit(1); // Exit if an invalid character is found
        }
    }

    return tokens;
}

enum class NodeType { VAR, NOT, AND, OR };

struct ASTNode {
    NodeType type;
    string value;
    ASTNode* left;
    ASTNode* right;

    ASTNode(NodeType t, const string& val = "") {
        type = t;
        value = val;
        left = nullptr;
        right = nullptr;
    }
};

ASTNode* parse(vector<Token>& tokens, int& pos) {
    if (pos >= tokens.size()) return nullptr;

    if (tokens[pos].type == TokenType::NOT_t) {
        pos++;
        ASTNode* operand = parse(tokens, pos);
        ASTNode* node = new ASTNode(NodeType::NOT);
        node->left = operand;
        return node;
    } 
    else if (tokens[pos].type == TokenType::VAR_t) {
        ASTNode* node = new ASTNode(NodeType::VAR, tokens[pos].value);
        pos++;
        return node;
    } 
    else if (tokens[pos].type == TokenType::LP_t) {
        pos++;
        ASTNode* left = parse(tokens, pos);
        Token opToken = tokens[pos];
        pos++;

        ASTNode* right = parse(tokens, pos);
        pos++; // Consume ')'

        NodeType opType = (opToken.type == TokenType::AND_t) ? NodeType::AND : NodeType::OR;
        ASTNode* node = new ASTNode(opType);
        node->left = left;
        node->right = right;
        return node;
    }
    return nullptr;
}

struct TseitinRes {
    int var;
    Formula formula;
};

int varCount = 1;
map<string, int> varMap;

TseitinRes encode(ASTNode* node) {

    TseitinRes res;
    if (node->type == NodeType::VAR) {
        if (varMap.find(node->value) != varMap.end()) {
            res.var = varMap[node->value];
        } else {
            res.var = varCount++;
            varMap[node->value] = res.var;
        }
    } 
    else if (node->type == NodeType::NOT) {
        TseitinRes sub = encode(node->left);
        res.var = varCount++;
        res.formula = sub.formula;
        res.formula.push_back({-res.var, -sub.var});
        res.formula.push_back({res.var, sub.var});
    } 
    else {
        TseitinRes leftRes = encode(node->left);
        TseitinRes rightRes = encode(node->right);
        res.var = varCount++;

        res.formula = leftRes.formula;
        res.formula.insert(res.formula.end(), rightRes.formula.begin(), rightRes.formula.end());

        if (node->type == NodeType::AND) {
            res.formula.push_back({-res.var, leftRes.var});
            res.formula.push_back({-res.var, rightRes.var});
            res.formula.push_back({res.var, -leftRes.var, -rightRes.var});
        } 
        else if (node->type == NodeType::OR) {
            res.formula.push_back({-leftRes.var, res.var});
            res.formula.push_back({-rightRes.var, res.var});
            res.formula.push_back({-res.var, leftRes.var, rightRes.var});
        }
    }
    return res;
}

void DIMACS(const TseitinRes& res) {
    cout << "p cnf " << (varCount - 1) << " " << res.formula.size() << endl;
    for (const Clause& clause : res.formula) {
        for (int lit : clause) {
            cout << lit << " ";
        }
        cout << "0\n";
    }
}

int main() {
    string formula;
    cout << "Enter a formula:\n";
    getline(cin, formula);

    vector<Token> tokens = tokenize(formula);
    int pos = 0;
    ASTNode* ast = parse(tokens, pos);

    TseitinRes result = encode(ast);
    result.formula.push_back({result.var});

    DIMACS(result);
    return 0;
}
