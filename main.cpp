#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <string>

using namespace std;

struct City { double x, y; };
using Tour = vector<int>;

static vector<City> parseTSP(const string& file) {
    ifstream in(file);
    vector<City> cities;
    string line;
    bool read = false;

    if (!in) {
        cerr << "Cannot open " << file << "\n";
        return cities;
    }

    while (getline(in, line)) {
        if (line.find("NODE_COORD_SECTION") != string::npos) {
            read = true;
            continue;
        }
        if (!read) continue;
        if (line.find("EOF") != string::npos) break;

        int id;
        double x, y;
        stringstream ss(line);
        if (ss >> id >> x >> y)
            cities.push_back({x, y});
    }
    return cities;
}

static vector<vector<int>> buildDist(const vector<City>& c) {
    int n = (int)c.size();
    vector<vector<int>> d(n, vector<int>(n, 0));

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            double dx = c[i].x - c[j].x;
            double dy = c[i].y - c[j].y;
            int w = (int)(sqrt(dx * dx + dy * dy) + 0.5);
            d[i][j] = d[j][i] = w;
        }
    }
    return d;
}

int tourCost(const Tour& t, const vector<vector<int>>& d) {
    long long s = 0;
    int n = (int)t.size();
    for (int i = 0; i < n - 1; i++)
        s += d[t[i]][t[i + 1]];
    s += d[t[n - 1]][t[0]];
    return (int)s;
}

int runGA(const vector<vector<int>>& d);
int runSA(const vector<vector<int>>& d);

int main() {
    vector<string> instances = {
        "pcb3038.tsp",
        "rl5915.tsp",
        "rl11849.tsp",
        "usa13509.tsp"
    };

    cout << left << setw(14) << "Instance"
         << setw(14) << "GA"
         << setw(14) << "SA" << "\n";

    for (const auto& f : instances) {
        cout << "Solving " << f << "...\n";

        auto cities = parseTSP(f);
        if (cities.empty()) continue;

        auto d = buildDist(cities);

        int ga = runGA(d);
        int sa = runSA(d);

        cout << left << setw(14) << f
             << setw(14) << ga
             << setw(14) << sa << "\n\n";
    }
    return 0;
}
