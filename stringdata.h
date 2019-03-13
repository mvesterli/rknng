#include <sstream>
#include <string>
#include <cstring>
#include <vector>
#include <algorithm>
using namespace std;


DataSet* loadStringData(std::string infname) {
    std::string line;
    std::ifstream infile(infname);
    DataSet* sd = (DataSet*) malloc(sizeof(DataSet));
    sd->strings = new vector<string>;
    sd->type=2; // String data
    int numLines = 0;

    while (std::getline(infile, line))
    {
        sd->strings->push_back(line);
        numLines++;
    }
    sd->size = numLines;
    return sd;
}

// Implementation copied from:
// https://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Dice%27s_coefficient#C
double dice_match(const char *string1, const char *string2) {

    //check fast cases
    if (((string1 != NULL) && (string1[0] == '\0')) ||
        ((string2 != NULL) && (string2[0] == '\0'))) {
        return 0;
    }
    if (string1 == string2) {
        return 1;
    }

    size_t strlen1 = strlen(string1);
    size_t strlen2 = strlen(string2);
    if (strlen1 < 2 || strlen2 < 2) {
        return 0;
    }

    size_t length1 = strlen1 - 1;
    size_t length2 = strlen2 - 1;

    double matches = 0;
    unsigned int i = 0, j = 0;

    //get bigrams and compare
    while (i < length1 && j < length2) {
        char a[3] = {string1[i], string1[i + 1], '\0'};
        char b[3] = {string2[j], string2[j + 1], '\0'};
        int cmp = strcmp(a, b);
        if (cmp == 0) {
            matches += 2;
        }
        i++;
        j++;
    }

    return matches / (length1 + length2);
}


float dice_distance(const std::string& s1, const std::string& s2) {
    float ret = (float) (1.0-dice_match(s1.c_str(), s2.c_str()));
    return ret;
}


// Implementation copied from:
// https://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance#C++
unsigned int edit_distance(const std::string& s1, const std::string& s2)
{
    /*std::cout <<"s1:" << s1 << " s2:" << s2 << "\n";*/
	const std::size_t len1 = s1.size(), len2 = s2.size();
	std::vector<std::vector<unsigned int>> d(len1 + 1, std::vector<unsigned int>(len2 + 1));

	d[0][0] = 0;
	for(unsigned int i = 1; i <= len1; ++i) d[i][0] = i;
	for(unsigned int i = 1; i <= len2; ++i) d[0][i] = i;

	for(unsigned int i = 1; i <= len1; ++i)
		for(unsigned int j = 1; j <= len2; ++j)
                      // note that std::min({arg1, arg2, arg3}) works only in C++11,
                      // for C++98 use std::min(std::min(arg1, arg2), arg3)
                      d[i][j] = std::min({ d[i - 1][j] + 1, d[i][j - 1] + 1, d[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1) });
	return d[len1][len2];
}


void debugStringDataset(DataSet* DS) {
    for (int i=0;i<10 && i < DS->size;i++) {
        string stmp = DS->strings->at(i);
        printf("i=%d/%d: %s\n",i,DS->size,stmp.c_str());
    }

}


