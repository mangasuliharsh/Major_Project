from __future__ import annotations

from dataclasses import dataclass, field
from typing import Dict, Iterable, List, Sequence, Tuple
import random

try:
    from sklearn.ensemble import RandomForestRegressor  # type: ignore
except Exception:  # pragma: no cover
    RandomForestRegressor = None


class UtilityRegressor:
    """RF if available, otherwise dependency-free linear SGD fallback."""

    def __init__(self, seed: int = 7):
        self.seed = seed
        self._rf = RandomForestRegressor(n_estimators=80, random_state=seed) if RandomForestRegressor else None
        self._w: List[float] | None = None

    def fit(self, X: Sequence[Sequence[float]], y: Sequence[float]) -> None:
        if not X:
            return
        if self._rf is not None:
            self._rf.fit(X, y)
            return
        # Simple linear regression trained with SGD (no external deps)
        dim = len(X[0])
        w = [0.0 for _ in range(dim + 1)]  # bias + weights
        lr = 0.03
        for _ in range(120):
            for xi, yi in zip(X, y):
                pred = w[0] + sum(wk * xk for wk, xk in zip(w[1:], xi))
                err = pred - yi
                w[0] -= lr * err
                for k, xk in enumerate(xi, start=1):
                    w[k] -= lr * err * xk
            lr *= 0.99
        self._w = w

    def predict_one(self, x: Sequence[float]) -> float:
        if self._rf is not None:
            return float(self._rf.predict([x])[0])
        if self._w is None:
            return 0.0
        return self._w[0] + sum(wi * xi for wi, xi in zip(self._w[1:], x))


@dataclass
class QLearner:
    alpha: float = 0.2
    gamma: float = 0.85
    epsilon: float = 0.2
    q: Dict[Tuple[int, int], float] = field(default_factory=dict)

    def value(self, state: int, action: int) -> float:
        return self.q.get((state, action), 0.0)

    def pick(self, state: int, actions: Iterable[int]) -> int:
        actions = list(actions)
        if not actions:
            raise ValueError("No actions available")
        if random.random() < self.epsilon:
            return random.choice(actions)
        return max(actions, key=lambda a: self.value(state, a))

    def update(self, s: int, a: int, reward: float, next_state: int, next_actions: Iterable[int]) -> None:
        next_actions = list(next_actions)
        max_next = max((self.value(next_state, na) for na in next_actions), default=0.0)
        old = self.value(s, a)
        self.q[(s, a)] = old + self.alpha * (reward + self.gamma * max_next - old)
