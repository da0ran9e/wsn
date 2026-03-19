# Problem Formulation

> Draft section for Section III of the paper.  
> Focus: the main problem with a single UAV executing the GMC mission in scenario4.

---

## 1. Problem statement

Let $\mathcal{P} = \{p_1, p_2, \ldots, p_M\}$ denote the set of suspicious ground nodes selected by the base station after the startup discovery phase, where each $p_i \in \mathbb{R}^2$ is the planar position of a suspicious node. A single UAV departs from the base-station location $x_0$ and follows a sequence of aerial waypoints

$$
\mathcal{W} = (w_1, w_2, \ldots, w_T), \qquad w_t \in \mathbb{R}^2,
$$

with constant flight speed $v$ and fixed altitude $h$.

At each waypoint, the UAV broadcasts image fragments while moving. A suspicious node can receive fragments directly from the UAV if it lies inside the effective broadcast region of that waypoint, and it can further obtain missing fragments via intra-cell cooperation from neighboring suspicious nodes. The mission is considered complete when the designated suspicious seed node $p^* \in \mathcal{P}$ accumulates confidence at least equal to the alert threshold $\tau_{\mathrm{alert}}$.

Therefore, the path-planning task is to determine a waypoint sequence that enables the seed node to reach the alert state as early as possible.

---

## 2. Coverage model

For any candidate waypoint $c$, define its suspicious-node coverage set as

$$
\mathrm{CS}(c) = \bigl\{ p \in \mathcal{P} \mid d(c,p) \le R_b \bigr\},
$$

where $R_b$ is the UAV broadcast radius and $d(\cdot,\cdot)$ is the Euclidean distance in the horizontal plane.

If the UAV visits waypoint sequence $\mathcal{W}$, then the cumulative covered suspicious-node set after the first $t$ waypoints is

$$
\mathrm{Covered}(t) = \bigcup_{k=1}^{t} \mathrm{CS}(w_k).
$$

This coverage set plays two roles in the system:

- it determines which suspicious nodes can directly receive fragments from the UAV, and
- it determines the set of nodes that may later relay fragments toward the seed node through intra-cell cooperation.

Hence, broad early coverage is a useful surrogate for fast confidence accumulation at the seed node.

---

## 3. Exact mission objective

Let $C_{p^*}(t)$ denote the confidence value of the seed node at time $t$. In scenario4, $C_{p^*}(t)$ depends on three coupled mechanisms:

- direct fragment reception from the UAV,
- fragment sharing among ground nodes inside the same cell, and
- stochastic packet success under the physical-layer channel model.

The true mission completion time is therefore

$$
T_{\mathrm{complete}}(\mathcal{W}) = \inf \bigl\{ t \ge 0 \;\big|\; C_{p^*}(t) \ge \tau_{\mathrm{alert}} \bigr\}.
$$

The exact optimization problem can be written as

$$
\min_{\mathcal{W}} \; T_{\mathrm{complete}}(\mathcal{W}),
$$

subject to UAV motion and communication constraints:

$$
t_k = \sum_{j=0}^{k-1} \frac{d(w_j, w_{j+1})}{v}, \qquad w_0 = x_0,
$$

$$
w_k \in \mathcal{C}, \qquad k = 1,2,\ldots,T,
$$

where $\mathcal{C}$ is the set of candidate waypoints considered by the planner.

This exact formulation is difficult to solve directly because $T_{\mathrm{complete}}$ depends on stochastic packet delivery, repeated broadcast opportunities, and cooperative fragment exchange among ground nodes.

---

## 4. Deterministic surrogate used for planning

To obtain a tractable planning problem, the system uses a deterministic surrogate based on geometric coverage. The intuition is that if the UAV visits waypoints that collectively cover all suspicious nodes while keeping travel cost small, then the seed node will receive fragments quickly either directly or indirectly through cooperation.

Accordingly, we require the planned route to cover the suspicious set:

$$
\bigcup_{t=1}^{T} \mathrm{CS}(w_t) \supseteq \mathcal{P}.
$$

Among all waypoint sequences satisfying this coverage constraint, we minimize travel time:

$$
\min_{w_1, \ldots, w_T} \; \hat{T}(\mathcal{W})
= \frac{1}{v} \sum_{t=0}^{T-1} d(w_t, w_{t+1}),
$$

with $w_0 = x_0$.

This surrogate objective is consistent with the implementation of scenario4 for two reasons:

- travel time determines how quickly the UAV can expose suspicious nodes to fragment broadcasts, and
- broader coverage increases the number of cooperative relay opportunities that can push the seed node toward $\tau_{\mathrm{alert}}$.

---

## 5. Optimization interpretation

The resulting planning problem is a variant of a **minimum-cost set-cover tour**:

- each waypoint $w$ corresponds to a set $\mathrm{CS}(w)$ of suspicious nodes it can cover,
- the union of selected sets must cover all suspicious nodes, and
- the total travel cost between consecutive selected waypoints must be minimized.

This problem combines the combinatorial structure of **set cover** with the routing structure of a **traveling tour**, and is NP-hard. Consequently, scenario4 adopts a greedy approximation strategy instead of exact global optimization.

---

## 6. Bridge to GMC

The GMC algorithm used in scenario4 solves the surrogate problem by repeatedly selecting the candidate waypoint that offers the best balance between:

- **coverage gain**: the number of newly covered suspicious nodes, and
- **travel cost**: the flight time required to reach that waypoint from the current UAV position.

This leads naturally to the scoring rule introduced in the next section:

$$
\mathrm{score}(c) = \frac{|\mathrm{CS}(c) \setminus \mathrm{Covered}|}{(d(x_t,c)/v)^\alpha + \varepsilon},
$$

where $\alpha$ controls the relative emphasis on short travel versus large coverage gain.

---

## 7. Short paper-ready version

The following paragraph is a compact version that can be inserted into `paper.tex` with minimal editing:

> Let $\mathcal{P} = \{p_1,\ldots,p_M\}$ be the positions of suspicious nodes and let $x_0$ denote the UAV starting position. For any candidate waypoint $c$, define its coverage set as $\mathrm{CS}(c)=\{p\in\mathcal{P}\mid d(c,p)\le R_b\}$, where $R_b$ is the effective UAV broadcast radius. The exact objective is to find a waypoint sequence $\mathcal{W}$ that minimizes the mission completion time $T_{\mathrm{complete}}(\mathcal{W})$, defined as the first time at which the suspicious seed node reaches confidence at least $\tau_{\mathrm{alert}}$. Because this completion time depends on stochastic packet reception and cooperative fragment forwarding, we adopt a deterministic surrogate: choose a waypoint sequence whose cumulative coverage satisfies $\cup_{t=1}^{T}\mathrm{CS}(w_t)\supseteq\mathcal{P}$ while minimizing total flight time $\hat{T}(\mathcal{W})=(1/v)\sum_{t=0}^{T-1} d(w_t,w_{t+1})$. This yields a minimum-cost set-cover tour problem, which is NP-hard and motivates the greedy GMC heuristic.
