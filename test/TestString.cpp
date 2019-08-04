#include <ate/ate.hpp>

using namespace std;
using namespace ate;



int main() 
{

    char str1[] = "    ";
    char str2[] = "    ";
    char* s1 = trimLeft(str1);
    trimRight(str2);

    cout << "str1:" << s1 << "|" << endl;
    cout << "str2:" << str2 << "|" << endl;

    char str3[] = "   hello world!    ";
    char* s3 = trim(str3);
    cout << "str3:" << s3 << "|" << endl;

    char str4[] = "   hello  world!    ";
    vector<char*> tokens;
    tokenize(tokens, str4);
    for (size_t i = 0; i < tokens.size(); ++i)
        cout << tokens[i] << "|" << endl;

    cout << "------------------------------------" << endl;
    tokens.clear();
    char str5[] = " hello  world!    ";
    split(tokens, str5);
    for (size_t i = 0; i < tokens.size(); ++i)
        cout << tokens[i] << "|" << endl;
}
