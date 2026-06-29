#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <cmath>
#include <climits>

using namespace std;
using Tour = vector<int>;

int tourCost(const Tour&, const vector<vector<int>>&);

static Tour nearestNeighbor(const vector<vector<int>>& d, mt19937& rng) {
    int n = (int)d.size();
    Tour t;
    vector<char> used(n, 0);

    int cur = rng() % n;
    used[cur] = 1;
    t.push_back(cur);

    for (int i = 1; i < n; i++) {
        int best = -1, bestD = INT_MAX;
        for (int j = 0; j < n; j++) {
            if (!used[j] && d[cur][j] < bestD) {
                bestD = d[cur][j];
                best = j;
            }
        }
        cur = best;
        used[cur] = 1;
        t.push_back(cur);
    }
    return t;
}

static void twoOptFull(Tour& r, const vector<vector<int>>& d) {
    int n = (int)r.size();
    bool improved = true;

    while (improved) {
        improved = false;
        for (int i = 0; i < n - 1; i++) {
            for (int j = i + 2; j < n; j++) {
                if (i == 0 && j == n - 1) continue;

                int a = r[i];
                int b = r[i + 1];
                int c = r[j];
                int e = r[(j + 1) % n];

                if (d[a][c] + d[b][e] < d[a][b] + d[c][e]) {
                    reverse(r.begin() + i + 1, r.begin() + j + 1);
                    improved = true;
                }
            }
        }
    }
}

int runSA(const vector<vector<int>>& d) {
    int n = (int)d.size();
    mt19937 rng(123);

    Tour cur = nearestNeighbor(d, rng);

    if (n <= 3000)
        twoOptFull(cur, d);

    int curCost = tourCost(cur, d);
    int bestCost = curCost;
    Tour best = cur;

    double T = (n < 1000 ? 3000.0 : 1500.0);
    double alpha = 0.9995;

    int iters =
    (n < 300   ? 2000 :
     n < 1000  ? 2000 :
     n < 3000  ? 1500 :
                  600);

    uniform_real_distribution<double> U(0.0, 1.0);

    for (int it = 0; it < iters; it++) {
        Tour neigh = cur;

        int i = rng() % (n - 1);
        int j = i + 1 + (rng() % (n - i - 1));
        reverse(neigh.begin() + i, neigh.begin() + j);

        if (n <= 3000)
            twoOptFull(neigh, d);

        int neighCost = tourCost(neigh, d);
        int delta = neighCost - curCost;

        if (delta < 0 || U(rng) < exp(-delta / T)) {
            cur = neigh;
            curCost = neighCost;
            if (curCost < bestCost) {
                bestCost = curCost;
                best = cur;
            }
        }

        T *= alpha;
        if (T < 1e-6) T = 1e-6;
    }

    return bestCost;
}

