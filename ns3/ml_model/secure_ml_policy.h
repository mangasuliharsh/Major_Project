#pragma once

#include "../network/wsn_network.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <set>
#include <utility>
#include <vector>
#include <random>
#include <cmath>

namespace wsn {

class SecureMlPolicy
{
public:
  void InitializeAttacks(uint32_t nNodes, double attackFraction, uint32_t seed)
  {
    m_sybil.clear();
    m_sinkhole.clear();
    m_selective.clear();

    uint32_t k = static_cast<uint32_t>(std::floor(nNodes * attackFraction));
    if (k == 0)
      return;

    std::vector<uint32_t> ids;
    for (uint32_t i = 1; i < nNodes; ++i)
      ids.push_back(i);

    std::mt19937 gen(seed + 997);
    std::shuffle(ids.begin(), ids.end(), gen);
    ids.resize(std::min<uint32_t>(k, ids.size()));

    uint32_t split = std::max<uint32_t>(1, ids.size() / 3);
    for (uint32_t i = 0; i < ids.size(); ++i)
    {
      if (i < split)
        m_sybil.insert(ids[i]);
      else if (i < 2 * split)
        m_sinkhole.insert(ids[i]);
      else
        m_selective.insert(ids[i]);
    }
  }

  bool ShouldSelectiveDrop(uint32_t nodeId, uint32_t seed, uint32_t round, uint32_t hop) const
  {
    if (!m_selective.count(nodeId))
      return false;
    std::mt19937 gen(seed + round * 131 + hop * 17 + nodeId);
    std::uniform_real_distribution<double> dis(0.0, 1.0);
    return dis(gen) < 0.45;
  }

  void UpdateTrust(std::vector<NodeState>& state, uint32_t nodeId, bool forwarded) const
  {
    double delta = forwarded ? 0.02 : -0.08;
    if (m_sybil.count(nodeId))
      delta -= 0.02;
    state[nodeId].trust = std::clamp(state[nodeId].trust + delta, 0.0, 1.0);
  }

  uint32_t NextHopAodvLike(const WsnNetwork& net, uint32_t cur) const
  {
    const auto& adj = net.Adj();
    const auto& state = net.State();
    double bestScore = -1e9;
    uint32_t best = UINT32_MAX;
    for (auto n : adj.at(cur))
    {
      if (!state[n].alive)
        continue;
      double score = net.ProgressToSink(cur, n);
      if (m_sinkhole.count(n))
        score *= 1.8;
      if (score > bestScore)
      {
        bestScore = score;
        best = n;
      }
    }
    return best;
  }

  uint32_t NextHopLeachLike(const WsnNetwork& net, uint32_t cur) const
  {
    const auto& adj = net.Adj();
    const auto& state = net.State();
    double bestScore = -1e9;
    uint32_t best = UINT32_MAX;
    for (auto n : adj.at(cur))
    {
      if (!state[n].alive)
        continue;
      double score = 0.6 * state[n].energy + 0.4 * net.ProgressToSink(cur, n);
      if (m_sinkhole.count(n))
        score *= 1.8;
      if (score > bestScore)
      {
        bestScore = score;
        best = n;
      }
    }
    return best;
  }

  uint32_t NextHopSecureMl(WsnNetwork& net, uint32_t cur)
  {
    const auto& adj = net.Adj();
    auto& state = net.State();

    double bestScore = -1e9;
    uint32_t best = UINT32_MAX;
    for (auto n : adj.at(cur))
    {
      if (!state[n].alive || state[n].energy <= 0.15 || state[n].trust < 0.3)
        continue;

      double energyFeature = state[n].energy;
      double degreeFeature = static_cast<double>(adj.at(n).size());
      double progressFeature = net.ProgressToSink(cur, n);
      double linkFeature = 1.0 / (1.0 + net.Distance(cur, n));
      double trustFeature = state[n].trust;

      double uHat = 0.35 * energyFeature + 0.25 * progressFeature + 0.10 * degreeFeature +
                    0.10 * linkFeature + 0.20 * trustFeature;

      auto key = std::make_pair(cur, n);
      double qVal = m_q.count(key) ? m_q[key] : 0.0;
      double risk = 1.0 - trustFeature;
      double lb = energyFeature;
      double score = 0.6 * uHat + 0.4 * qVal - 0.6 * risk + 0.3 * lb;

      if (score > bestScore)
      {
        bestScore = score;
        best = n;
      }
    }
    return best;
  }

  void QUpdate(uint32_t s, uint32_t a, uint32_t s2, const std::map<uint32_t, std::vector<uint32_t>>& adj, double reward)
  {
    std::pair<uint32_t, uint32_t> key{s, a};
    double old = m_q.count(key) ? m_q[key] : 0.0;

    double maxNext = 0.0;
    auto it = adj.find(s2);
    if (it != adj.end())
    {
      for (auto n : it->second)
      {
        std::pair<uint32_t, uint32_t> nk{s2, n};
        if (m_q.count(nk))
          maxNext = std::max(maxNext, m_q[nk]);
      }
    }

    const double alpha = 0.2;
    const double gamma = 0.85;
    m_q[key] = old + alpha * (reward + gamma * maxNext - old);
  }

private:
  std::set<uint32_t> m_sybil;
  std::set<uint32_t> m_sinkhole;
  std::set<uint32_t> m_selective;
  std::map<std::pair<uint32_t, uint32_t>, double> m_q;
};

} // namespace wsn
