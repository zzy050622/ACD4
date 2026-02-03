#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <chrono>
#include <unordered_map>
#include <algorithm>
#include <cstring>

using namespace std;
using namespace chrono;

// 结构体：适配4字段格式（时间+姓名+序列号+描述），保留原始行号
// Struct: Adapt to 4-field format (time+name+serial+description), retain original line number
struct Record {
    string time;          // 第1字段：时间（如 06.10.2016）| Field 1: Time (e.g., 06.10.2016)
    string name;          // 第2字段：姓名（ФИО，如 Lopez, Anthony）| Field 2: Full Name (FIO, e.g., Lopez, Anthony)
    int serialNum;        // 第3字段：序列号（数值类型）| Field 3: Serial number (numeric type)
    string description;   // 第4字段：描述（文本）| Field 4: Description (text)
    int originalLine;     // 原始行号（从1开始）| Original line number (starts from 1)
};

// ---------------------- 拉宾-卡普算法（多模板哈希匹配）----------------------
// 拉宾-卡普算法：多模板匹配，返回文本中匹配的模板数量是否达标
// Rabin-Karp Algorithm: Multi-pattern matching, returns if matched patterns meet target
int rabinKarpSearch(const string& text, const vector<string>& patterns, bool isCountOccurrences, int targetCount) {
    if (patterns.empty() || text.empty()) return 0;

    const uint64_t base = 911382629;  // 哈希基数（大质数）| Hash base (large prime)
    const uint64_t mod = 1000000007;  // 取模值（避免溢出）| Mod value (avoid overflow)
    int maxPatternLen = 0;
    for (const auto& p : patterns) {
        maxPatternLen = max(maxPatternLen, (int)p.size());
    }
    int textLen = text.size();
    if (maxPatternLen > textLen) return 0;

    // 预处理：计算所有模板的哈希值 | Preprocess: Calculate hash of all patterns
    unordered_map<uint64_t, vector<string>> patternHashMap;
    for (const auto& p : patterns) {
        int pLen = p.size();
        uint64_t pHash = 0;
        for (char c : p) {
            pHash = (pHash * base + (uint64_t)c) % mod;
        }
        patternHashMap[pHash].push_back(p);
    }

    // 预处理：计算 base^maxPatternLen mod mod | Preprocess: Calculate base^maxPatternLen mod mod
    uint64_t basePower = 1;
    for (int i = 0; i < maxPatternLen - 1; ++i) {
        basePower = (basePower * base) % mod;
    }

    // 计算文本第一个窗口的哈希值 | Calculate hash of first text window
    uint64_t windowHash = 0;
    for (int i = 0; i < maxPatternLen; ++i) {
        windowHash = (windowHash * base + (uint64_t)text[i]) % mod;
    }

    unordered_map<string, int> patternOccurrences;  // 模板出现次数 | Pattern occurrence count
    int matchedPatternCount = 0;                    // 匹配的模板数量 | Number of matched patterns

    // 滑动窗口遍历文本 | Slide window through text
    for (int i = 0; i <= textLen - maxPatternLen; ++i) {
        if (patternHashMap.count(windowHash)) {
            // 哈希匹配，验证字符是否完全一致 | Hash match, verify character consistency
            for (const auto& p : patternHashMap[windowHash]) {
                int pLen = p.size();
                if (pLen != maxPatternLen) continue;
                bool isMatch = true;
                for (int j = 0; j < pLen; ++j) {
                    if (text[i + j] != p[j]) {
                        isMatch = false;
                        break;
                    }
                }
                if (isMatch) {
                    if (patternOccurrences[p] == 0) matchedPatternCount++;
                    patternOccurrences[p]++;
                }
            }
        }

        // 更新窗口哈希值（滑动）| Update window hash (slide)
        if (i < textLen - maxPatternLen) {
            windowHash = ((windowHash - (uint64_t)text[i] * basePower) % mod + mod) % mod;
            windowHash = (windowHash * base + (uint64_t)text[i + maxPatternLen]) % mod;
        }
    }

    // 返回是否达标 | Return if target is met
    if (isCountOccurrences) {
        return (patterns.size() == 1) ? (patternOccurrences[patterns[0]] >= targetCount ? 1 : 0) : 0;
    }
    else {
        return matchedPatternCount >= targetCount ? 1 : 0;
    }
}

// ---------------------- 博伊尔-穆尔算法（单模板高效匹配）----------------------
// 预处理坏字符表 | Preprocess bad character table
void preprocessBadChar(const string& pattern, vector<int>& badChar) {
    int pLen = pattern.size();
    badChar.resize(256, -1);
    for (int i = 0; i < pLen; ++i) {
        badChar[(unsigned char)pattern[i]] = i;  // 记录字符最右侧位置 | Record rightmost position of character
    }
}

// 预处理好后缀表 | Preprocess good suffix table
void preprocessGoodSuffix(const string& pattern, vector<int>& goodSuffix) {
    int pLen = pattern.size();
    goodSuffix.resize(pLen, 0);
    vector<int> suffix(pLen, 0);
    int lastPos = pLen - 1;

    for (int i = pLen - 2; i >= 0; --i) {
        if (i > lastPos && suffix[i + pLen - 1 - lastPos] < i - lastPos) {
            suffix[i] = suffix[i + pLen - 1 - lastPos];
        }
        else {
            lastPos = min(i, lastPos);
            while (lastPos >= 0 && pattern[lastPos] == pattern[lastPos + pLen - 1 - i]) {
                lastPos--;
            }
            suffix[i] = i - lastPos;
        }
    }

    for (int i = 0; i < pLen; ++i) goodSuffix[i] = pLen;
    int j = 0;
    for (int i = pLen - 1; i >= 0; --i) {
        if (suffix[i] == i + 1) {
            while (j < pLen - 1 - i && goodSuffix[j] == pLen) {
                goodSuffix[j] = pLen - 1 - i;
                j++;
            }
        }
    }
    for (int i = 0; i < pLen - 1; ++i) {
        goodSuffix[pLen - 1 - suffix[i]] = pLen - 1 - i;
    }
}

// 博伊尔-穆尔算法：单模板匹配，返回出现次数是否达标 | Boyer-Moore: Single-pattern, return if occurrences meet target
int boyerMooreSearch(const string& text, const string& pattern, int targetCount) {
    int textLen = text.size();
    int pLen = pattern.size();
    if (pLen == 0 || textLen < pLen) return 0;

    vector<int> badChar(256, -1);
    vector<int> goodSuffix(pLen, 0);
    preprocessBadChar(pattern, badChar);
    preprocessGoodSuffix(pattern, goodSuffix);

    int i = 0;
    int occurrenceCount = 0;

    while (i <= textLen - pLen) {
        int j = pLen - 1;
        while (j >= 0 && text[i + j] == pattern[j]) j--;

        if (j < 0) {
            // 完全匹配，计数+1 | Full match, increment count
            occurrenceCount++;
            i += (i + pLen < textLen) ? goodSuffix[0] : 1;
        }
        else {
            // 按规则跳过 | Skip by rules
            int badStep = j - badChar[(unsigned char)text[i + j]];
            int goodStep = goodSuffix[j];
            i += max(badStep, goodStep);
        }
    }

    return occurrenceCount >= targetCount ? 1 : 0;
}

// ---------------------- 辅助函数：读取输入文件 ----------------------
// 读取4字段制表符分隔文件 | Read 4-field tab-separated file
bool readInputFile(const string& filename, int n, vector<Record>& data) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Failed to open input file " << filename << endl;
        return false;
    }

    data.clear();
    string line;
    int lineCount = 0;

    while (getline(file, line) && lineCount < n) {
        lineCount++;
        stringstream ss(line);
        string timeStr, nameStr, serialStr, descStr;

        // 按制表符拆分4个字段 | Split 4 fields by tab
        if (!getline(ss, timeStr, '\t') || !getline(ss, nameStr, '\t') ||
            !getline(ss, serialStr, '\t') || !getline(ss, descStr)) {
            cerr << "Error: Line " << lineCount << " format error (need 4 fields)" << endl;
            file.close();
            return false;
        }

        // 序列号转换为整数 | Convert serial number to integer
        int serialNum;
        try {
            serialNum = stoi(serialStr);
        }
        catch (const exception& e) {
            cerr << "Error: Line " << lineCount << " invalid serial number" << endl;
            file.close();
            return false;
        }

        // 存入结构体 | Store in struct
        Record record;
        record.time = timeStr;
        record.name = nameStr;
        record.serialNum = serialNum;
        record.description = descStr;
        record.originalLine = lineCount;
        data.push_back(record);
    }

    if (lineCount < n) {
        cerr << "Error: Input file has only " << lineCount << " lines (required " << n << ")" << endl;
        file.close();
        return false;
    }

    file.close();
    return true;
}

// ---------------------- 辅助函数：写入输出文件 ----------------------
// 写入匹配结果（表格格式）+ 耗时 | Write matched results (table) + elapsed time
void writeOutputFile(const string& filename, const vector<Record>& matchedRecords, double elapsedTime) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Failed to open output file " << filename << endl;
        return;
    }

    // 写入匹配记录（原始行号+4字段）| Write matched records (original line + 4 fields)
    for (const auto& record : matchedRecords) {
        file << record.originalLine << "\t" << record.time << "\t" << record.name
            << "\t" << record.serialNum << "\t" << record.description << endl;
    }

    // 最后一行写入耗时 | Write elapsed time in last line
    file << "Search Time: " << elapsedTime << " ms" << endl;
    file.close();
    cout << "Results saved to " << filename << endl;
}

// ---------------------- 主函数（流程调度+用户交互）----------------------
int main() {
    // 1. 控制台输入参数 | Console input parameters
    string inputFilename = "input.txt";  // 输入文件（需与代码同目录）| Input file (same directory as code)
    int n;                               // 处理行数（10≤n≤1000000）| Lines to process (10≤n≤1000000)
    int searchField;                     // 搜索字段（1=姓名，2=描述）| Search field (1=Name, 2=Description)
    int algorithmType;                   // 算法（1=拉宾-卡普，2=博伊尔-穆尔）| Algorithm (1=Rabin-Karp, 2=Boyer-Moore)
    int targetCount;                     // 匹配要求（次数/模板数）| Target count (occurrences/patterns)
    vector<string> patterns;             // 搜索模板 | Search patterns

    // 输入处理行数 | Input lines to process
    cout << "Please enter the number of lines to process (10 ≤ n ≤ 1000000): ";
    while (!(cin >> n) || n < 10 || n > 1000000) {
        cin.clear();
        cin.ignore(1024, '\n');
        cerr << "Invalid input! Enter integer 10~1000000: ";
    }

    // 输入搜索字段 | Input search field
    cout << "Select search field (1=Name/FIO, 2=Description): ";
    while (!(cin >> searchField) || (searchField != 1 && searchField != 2)) {
        cin.clear();
        cin.ignore(1024, '\n');
        cerr << "Invalid input! Select 1 (Name) or 2 (Description): ";
    }

    // 输入算法类型 | Input algorithm type
    cout << "Select algorithm (1=Rabin-Karp, 2=Boyer-Moore): ";
    while (!(cin >> algorithmType) || (algorithmType != 1 && algorithmType != 2)) {
        cin.clear();
        cin.ignore(1024, '\n');
        cerr << "Invalid input! Select 1 (Rabin-Karp) or 2 (Boyer-Moore): ";
    }

    // 输入搜索模板和匹配要求 | Input patterns and target count
    cin.ignore(1024, '\n');  // 清除缓冲区 | Clear buffer
    if (algorithmType == 1) {
        // 拉宾-卡普：多模板（空行结束）| Rabin-Karp: Multi-pattern (end with empty line)
        cout << "Rabin-Karp: Enter patterns (one per line, end with empty line): " << endl;
        string pattern;
        while (getline(cin, pattern)) {
            if (pattern.empty()) break;
            patterns.push_back(pattern);
        }
        if (patterns.empty()) {
            cerr << "Error: At least one pattern is required!" << endl;
            return 1;
        }
        // 输入需匹配的模板数量 | Input target number of patterns
        cout << "Enter minimum number of patterns to match: ";
        while (!(cin >> targetCount) || targetCount < 1 || targetCount > patterns.size()) {
            cin.clear();
            cin.ignore(1024, '\n');
            cerr << "Invalid input! Enter 1~" << patterns.size() << ": ";
        }
    }
    else {
        // 博伊尔-穆尔：单模板 | Boyer-Moore: Single-pattern
        cout << "Boyer-Moore: Enter pattern: ";
        string pattern;
        getline(cin, pattern);
        patterns.push_back(pattern);
        // 输入需出现的次数 | Input target occurrences
        cout << "Enter minimum number of occurrences: ";
        while (!(cin >> targetCount) || targetCount < 1) {
            cin.clear();
            cin.ignore(1024, '\n');
            cerr << "Invalid input! Enter positive integer: ";
        }
    }

    // 2. 读取输入文件 | Read input file
    vector<Record> data;
    if (!readInputFile(inputFilename, n, data)) {
        return 1;  // 读取失败退出 | Exit if read fails
    }

    // 3. 执行搜索（计时）| Execute search (with timing)
    vector<Record> matchedRecords;
    auto start = high_resolution_clock::now();  // 开始计时 | Start timing

    for (const auto& record : data) {
        string targetText = (searchField == 1) ? record.name : record.description;
        int isMatched = 0;

        if (algorithmType == 1) {
            isMatched = rabinKarpSearch(targetText, patterns, false, targetCount);
        }
        else {
            isMatched = boyerMooreSearch(targetText, patterns[0], targetCount);
        }

        if (isMatched) {
            matchedRecords.push_back(record);
        }
    }

    auto end = high_resolution_clock::now();
    double elapsedTime = duration<double, milli>(end - start).count();  // 耗时（毫秒）| Elapsed time (ms)

    // 4. 写入输出文件 | Write output file
    string outputFilename = (algorithmType == 1) ? "rabin_karp_results.txt" : "boyer_moore_results.txt";
    writeOutputFile(outputFilename, matchedRecords, elapsedTime);

    // 5. 打印统计信息 | Print statistics
    cout << "\nSearch Completed!" << endl;
    cout << "Matched records: " << matchedRecords.size() << endl;
    cout << "Elapsed time: " << elapsedTime << " ms" << endl;

    return 0;
}