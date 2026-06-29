#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <climits>

using namespace std;
using Tour = vector<int>;

int tourCost(const Tour&, const vector<vector<int>>&);

/* ---------- Candidate list ---------- */

static vector<vector<int>> buildCandidates(const vector<vector<int>>& d, int K) {
    int n = (int)d.size();
    vector<vector<int>> cand(n);
    vector<pair<int,int>> tmp;

    for (int i = 0; i < n; i++) {
        tmp.clear();
        for (int j = 0; j < n; j++)
            if (i != j) tmp.push_back({d[i][j], j});

        int kk = min(K, (int)tmp.size());
        nth_element(tmp.begin(), tmp.begin() + kk, tmp.end());

        for (int k = 0; k < kk; k++)
            cand[i].push_back(tmp[k].second);
    }
    return cand;
}

/* ---------- Nearest Neighbor ---------- */

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

/* ---------- 2-opt ---------- */

static inline int delta2opt(const Tour& r, int i, int j,
                            const vector<vector<int>>& d) {
    int n = (int)r.size();
    int a = r[i];
    int b = r[i + 1];
    int c = r[j];
    int e = r[(j + 1) % n];
    return (d[a][c] + d[b][e]) - (d[a][b] + d[c][e]);
}

static void twoOpt(Tour& r,
                   const vector<vector<int>>& d,
                   const vector<vector<int>>& cand,
                   mt19937& rng,
                   int rounds)
{
    int n = (int)r.size();
    vector<int> pos(n);
    for (int i = 0; i < n; i++) pos[r[i]] = i;

    uniform_int_distribution<int> pickI(0, n - 2);

    for (int it = 0; it < rounds; it++) {
        int i = pickI(rng);
        for (int c : cand[r[i]]) {
            int j = pos[c];
            if (j <= i + 1) continue;
            if (i == 0 && j == n - 1) continue;

            if (delta2opt(r, i, j, d) < 0) {
                reverse(r.begin() + i + 1, r.begin() + j + 1);
                for (int k = i + 1; k <= j; k++)
                    pos[r[k]] = k;
                break;
            }
        }
    }
}

/* ---------- Crossover & mutation ---------- */

static Tour ox(const Tour& p1, const Tour& p2, mt19937& rng) {
    int n = (int)p1.size();
    Tour c(n, -1);
    vector<char> used(n, 0);

    uniform_int_distribution<int> D(0, n - 1);
    int a = D(rng), b = D(rng);
    if (a > b) swap(a, b);

    for (int i = a; i <= b; i++) {
        c[i] = p1[i];
        used[c[i]] = 1;
    }

    int k = (b + 1) % n;
    for (int i = 0; i < n; i++) {
        int v = p2[(b + 1 + i) % n];
        if (!used[v]) {
            c[k] = v;
            used[v] = 1;
            k = (k + 1) % n;
        }
    }
    return c;
}

static void mutate(Tour& t, mt19937& rng) {
    uniform_int_distribution<int> D(0, (int)t.size() - 1);
    int i = D(rng);
    int j = D(rng);
    if (i != j) swap(t[i], t[j]);
}

/* ---------- GA ---------- */

struct Ind { Tour t; int cost; };

static int tournament(const vector<Ind>& pop, mt19937& rng) {
    uniform_int_distribution<int> D(0, (int)pop.size() - 1);
    int best = D(rng);
    for (int i = 0; i < 2; i++) {
        int j = D(rng);
        if (pop[j].cost < pop[best].cost)
            best = j;
    }
    return best;
}

int runGA(const vector<vector<int>>& d) {
    int n = (int)d.size();
    mt19937 rng(42);

    int K = (n < 300 ? 30 : (n < 1000 ? 25 : 22));
    auto cand = buildCandidates(d, K);

    const int POP = 150;
    const int GENS = 2000;

    vector<Ind> pop(POP);

    /* ----- Initialization ----- */
    for (int i = 0; i < POP; i++) {
        pop[i].t = nearestNeighbor(d, rng);
        twoOpt(pop[i].t, d, cand, rng, (n < 1500 ? 3000 : 5000));
        pop[i].cost = tourCost(pop[i].t, d);
    }

    uniform_real_distribution<double> U(0.0, 1.0);

    /* ----- Evolution ----- */
    for (int g = 0; g < GENS; g++) {

        sort(pop.begin(), pop.end(),
             [](const Ind& a, const Ind& b) {
                 return a.cost < b.cost;
             });

        int ELITE = 10 - g / 400;
        if (ELITE < 2) ELITE = 2;

        if (g % 50 == 0) {
            for (int i = 0; i < ELITE; i++) {
                twoOpt(pop[i].t, d, cand, rng,
                       (n < 1500 ? 3000 : 6000));
                pop[i].cost = tourCost(pop[i].t, d);
            }
        }

        vector<Ind> next;
        for (int i = 0; i < ELITE; i++)
            next.push_back(pop[i]);

        while ((int)next.size() < POP) {
            int p1 = tournament(pop, rng);
            int p2 = tournament(pop, rng);
            if (p1 == p2) p2 = tournament(pop, rng);

            Tour child = ox(pop[p1].t, pop[p2].t, rng);

            /* ----- Adaptive mutation ----- */
            double pm = (n > 3000 ? 0.20 : 0.15);
            if (g > GENS * 0.6) pm *= 1.5;
            if (pm > 0.5) pm = 0.5;

            if (U(rng) < pm)
                mutate(child, rng);

            twoOpt(child, d, cand, rng,
                   (n < 1500 ? 1200 : 2000));

            next.push_back({child, tourCost(child, d)});
        }

        pop.swap(next);
    }

    return min_element(pop.begin(), pop.end(),
        [](const Ind& a, const Ind& b){
            return a.cost < b.cost;
        })->cost;
}
