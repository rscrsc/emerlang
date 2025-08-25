# emerLang - An ECS-based Emulater to Explore the Emergence of Language

## TODO List
- [ ] A seperate WebGL renderer (instead of imgui BackgroundList)
- [ ] Zero copy SoA indexing

## 1）交流需求从哪来（第一性原理）
用最小假设，让“说话”自带动力：
- **部分可观测环境**：视觉半径有限、地形遮挡、资源/威胁在视野外（信息不对称）。
- **协作任务需要多人**：某些资源需要≥2个体同时到场才能采集/搬运；单人收益极低。
- **时间敏感与移动目标**：食物/猎物会移动或快速腐败；早到者可得更大收益。
- **声音的物理特性**：传播快、穿透遮挡，但有**能量成本**与**被捕食者侦测风险**。
- **适应度 = 生存 + 繁殖成功率**：信息共享能显著提高适应度，但要付声学成本与风险。
> 语言 = 在“信息价值 > 发声成本/风险”的域里自发出现的**低成本协调协议**。

## 2）ECS 设计（最小可跑）
### 实体（Entities）
- **Agent**：个体
- **Resource**：可采集资源（类型/数量/更新速度）
- **Predator**（可选）：捕食者/威胁
- **Obstacle**：遮挡/反射体
- **SoundWave**：声波包（事件化）
- **Group**（可选）：临时协作组

### 组件（Components）
#### Agent 相关
- `Position(x,y)`，`Velocity(vx,vy)`
- `Energy`：能量；发声与移动消耗
- `Needs`：饥饿值、繁殖阈值
- `Perception`：
  - `VisionRange`（遮挡影响）
  - `Hearing`：滤波器组/阈值、方位估计开关
- `VocalTract`：可发频段[minF,maxF]、最大振幅、包络形状集（离散/连续）
- `Lexicon`：信号↔意义（向量量化码本/序列概率表）
- `Policy`：行动/发声策略参数（可学习）
- `Social`：信任/亲缘/近邻列表
- `Memory`：短期（最近 N 条听到/说出）、长期（已对齐的映射）

#### 资源/环境
- `ResourceType`，`Quantity`，`RegenerationRate`
- `AcousticMedium`：声速 c、衰减系数 α、环境噪声 σ、风向 w
- `Occluder`：吸收/反射系数（0–1）

#### 声波（事件）
- `SoundWave`：`origin`, `t_emit`, `f0`（主频）, `spec`（频谱向量/梅尔倒谱系数）, `amp`, `duration`
- `Payload`（*不直接给意义*，只给物理信号特征；意义靠代理自己学）
- `TTL`：传播寿命（距离/时间耗尽即销毁）

### 系统（Systems）
**更新顺序建议：**
1. `NeedSystem`：代谢、饥饿上升、能量扣除
2. `PerceptionGatherSystem`
   - 视觉采样（遮挡）
   - **听觉接收**（见 3）
3. `SoundPropagationSystem`
   - 基于栅格或空间索引的**事件传播**：  
     - 距离衰减：\( A(d)=A_0 e^{-\alpha d} \)  
     - 噪声：加入 \( \mathcal{N}(0,\sigma^2) \)  
     - 延时：到达时间 \( t = d/c \)（可选 ITD/ILD 作方位估计）
     - 遮挡：按吸收/反射系数修正
4. `DecisionSystem`（RL/演化规则）
   - 采集/移动/结伴/发声/沉默
5. `VocalizationSystem`
   - 若选择发声：生成 `SoundWave` 事件（频率/时长/包络/序列）
   - 发声能量成本 & 风险（吸引 Predator）
6. `InteractionSystem`
   - 结对搬运/协作触发奖励
7. `LearningAlignmentSystem`
   - **信号→意义**的对齐更新（见 4）
8. `ResourceRegenerationSystem` / `PredatorSystem`
9. `LoggingSystem`（详见 6）

## 3）声学最小物理模型
- **传播**：事件驱动而非全场波动（性能友好）
- **字段**：`{t_emit, origin, f0, spec(梅尔/倒谱), amp, duration}`
- **到达判定**：在听觉半径内且 `SNR > θ` 被接收
- **定位（可选）**：以 ITD/ILD 粗估方位，误差与噪声相关
- **组合消息**：允许序列化（音节序列），中间静默作分隔；这为**语法/组合性**留下空间

## 4）“语言”如何涌现（不预设词典）
给一个**对齐—强化**闭环，让“信号—意义”映射自己长出来：

#### 4.1 意义的来源（不写死词表）
- **意义并不直接是“词*义*”**，而是**情境中的“可用信息”**：如
  - 资源类别/方位（离散/连续标签）
  - 协作召集（需要几人/到哪儿/何时）
  - 危险预警（威胁等级/方向）
- 这些“语义变量”来自环境真实态（ground truth），**但代理看不全**（部分可观测），只有在**任务成功**后才能通过奖励反推“这条信号有用”。

#### 4.2 听觉 → 类别化（范畴化）
- 代理把连续声学特征 `spec` 用**向量量化（VQ）/聚类（k-means/在线 VQ）**变成**离散“音位”**（codebook）
- 音位序列 = “音节/词”的雏形

#### 4.3 映射学习（对齐）
- 采用**策略梯度/Actor–Critic**或**带记忆的进化策略**：
  - 状态：自身观测 + 最近听到的音位序列
  - 动作：移动/协作/发声（选择音位序列）
  - 奖励：能量净增长、协作成功、避免威胁
- 同时维护 `Lexicon`：音位序列 ↔ 语义变量 的**贝叶斯表/Q表**（置信度）
- **对齐触发**：当听到 X 后执行策略 Y 得到正奖励 → 提升 `P(Meaning|Signal)`；若误导/无效则衰减

#### 4.4 社会对齐（收敛成“语言”）
- **模仿偏好**：对高收益个体/亲近个体的信号更易被采纳
- **代际传承**：出生时继承部分 `Lexicon`（带噪复制→可进化）
- **成本—收益门槛**：高成本/高风险下，只保留**信息价值高**的信号（形成稀疏词汇、Zipf 近似）

> 不预设任何单词或语法；仅给**听觉离散化 + 强化对齐 + 模仿传承**，语言结构就能自己冒出来。

## 5）最小实验脚本（可一步步跑）
#### 实验 A：协作采集（两人抬）
- **设定**：资源 R 需要 ≥2 体同到并交互才产出，且位置通常在多数体的视野外
- **期望**：出现“召集/指路”信号；先有“到我这儿”，后出现“方向+距离”的组合编码

#### 实验 B：捕食者预警
- **设定**：Predator 朝听到声源移动；但预警能让族群逃离，净收益仍为正
- **期望**：形成**短而急促**的高频警报；在高风险区“沉默”策略上升（语用条件性）

#### 实验 C：多资源类型的指称
- **设定**：R 有类型（果/肉/水），最佳处理方式不同
- **期望**：音位组合与资源类型一一对应；出现**组合性**（类型 + 方位）

## 6）可观测与记录（强可视化/可复现）
### 6.1 事件日志（行式）
`UtterLog.csv`
```
tick, speaker_id, pos_x, pos_y, seq, f0, dur, amp, recv_ids[], snr[], local_state_hash, reward_delta_after_Δt
```
`AlignLog.csv`
```
tick, agent_id, seq, inferred_meaning, conf_before, conf_after, context_signature
```
`TaskLog.csv`
```
tick, task_type, participants[], success, reward_total, loc
```

### 6.2 指标（按回合输出）
- **通信成功率**：听到 X 后在目标 Δt 内完成任务的概率
- **互信息** \( I(\text{Signal}; \text{Meaning}) \)
- **拓扑相似度**（语音距离 vs 语义距离的相关性 → 组合性）
- **码本大小/熵/Zipf 曲线**（是否呈幂律）
- **对齐度**：不同个体 `Lexicon` 的 KL/JS 距离
- **语用效率**：每比特奖励/每声学能量的奖励

### 6.3 可视化（运行时）
- 地图：资源/代理/声波前沿（衰减圈）叠加
- 语音瀑布图（随机抽样若干发声）
- 词汇网络（节点=序列，边=同义/共现）
- “听到→行动”的**反应时直方图**
- 实时打印“Top-K 稳定映射”：`seq → {meaning: prob}`

## 7）性能与实现细节
- **空间索引**：声波传播和听觉接收用**网格/四叉树**过滤查询
- **事件池化**：`SoundWave` 走对象池，TTL 到期回收
- **传播近似**：首选**事件球壳**而不是全场波动；复杂场景再加反射
- **并行**：ECS 天然并行；感知/传播/学习分别跑 Job
- **平台**：Unity DOTS / Rust Bevy / WebGPU（浏览器演示）

## 8）参数与消融建议
- **去通信对照**：禁发声，测任务成功率基线
- **成本提高**：上调发声能量或捕食风险，看是否只保留高信息量词
- **视觉放宽**：扩大视距，验证“交流需求”随可观测性下降而增强
- **代际传承开关**：无继承时词汇稳定性下降

## 9）最小规则集（可直接照此实现）
- 预设：  
  1) 部分可观测环境 + 协作任务 + 声学成本/风险  
  2) 听觉离散化（VQ）与序列产生（可用简单马尔可夫）  
  3) 强化对齐（奖励由任务成功给出） + 模仿偏好 + 代际传承  
- 涌现：  
  - 词汇、警报与指称、组合性、语用调节、Zipf 近似、社群方言/对齐

------------------------

## 1）最小预设 vs. 可涌现

**必须预设（越少越好）**
- **物理层**：二维/三维空间；声速 \(c\)、衰减、噪声；发声有能量成本。
- **个体层**：  
  - 基本生存（能量、摄取、移动、繁殖、死亡）。  
  - 传感器（耳）：在位置 \(x\) 处感知到的声压随距离/障碍衰减；带噪。  
  - 执行器（喉）：能生成时频可调的短声段（频率、时长、振幅）。  
  - 有限记忆与简单强化学习（RL）/模仿机制（但**不**预设任何“词义表”）。
- **环境层**：资源稀缺、障碍、需要**协作**才能获得的高价值机会（见第 3 点）。

**允许涌现**
- 信号类别（“音位”原型）、信号-情境映射（“词义”）、轮流/打断、对齐/方言、语序偏好、社群专用“术语”等。

## 2）交流需求从哪来？（动机的第一性原理）

让**“不开口=吃亏”**成为事实：设置**必须协作**或**远距告警**才能提高存活率/收益的情境：
- **协作采集**：高能量资源需要 ≥2 个体同时抵达/按顺序操作（单人得不到）。  
- **空间遮挡**：视觉/嗅觉被障碍阻断，只有声波可跨障碍传递位置信息。  
- **掠食者预警**：先知先觉者若不喊叫，群体死亡率上升；喊叫虽有成本，但群体存活提高→亲缘/互惠提升**长期回报**。  
- **机会转瞬即逝**：资源短窗开放，信息扩散越快，群体收益越大。

**决策原则（个体是否发声）**  
发声当且仅当：  
\[
\mathbb{E}[\Delta R \mid \text{发声}] - \text{能量成本} - \text{暴露风险} > \tau
\]
其中 \(\Delta R\) 是“因交流带来的额外回报”，\(\tau\) 是个体风险阈值（可进化/学习）。

## 3）ECS 设计

### Entities
- **Agent**（个体）
- **SoundWave**（声波包/波前）
- **Resource**（资源点/事件）
- **Obstacle**（墙体/山体，反射/阻挡）
- **Predator/Event**（风险/机会）

### Components（示例字段）
- `Transform`：`pos, vel`
- `Energy`：`value, basal_metabolism`
- `Foraging`：`capacity, carrying`
- `Ear`：`bandwidth, sensitivity, snr_floor, directionality`
- `VocalTract`：`f_min, f_max, max_amp, cost_per_db, envelope_shape`
- `Memory`：`spectral_buffer[N_frames], episodic[K]`
- `Policy`：`θ`（RL 参数：发声/移动/协作策略）
- `Lexicon`（**不预设**内容，仅容器）：  
  - `prototypes`: 若干声学原型向量（连续时频向量/嵌入）  
  - `meanings`: 与“情境/动作/目标”的联结权重  
  - `alignment_score`：与邻居对齐度
- `GroupTag`：群体/亲缘标识（可为空）
- `SoundWave`：`origin, t_emit, spectrum_or_params, amp, phase`

### Systems（更新顺序建议）
1. **Environment/Event System**：生成/更新资源与风险、开窗时段。  
2. **Movement/Foraging System**：个体搜寻、采集、协作判定。  
3. **Vocalization System**：根据 \(\mathbb{E}[\Delta R]\) 决策是否发声，生成 `SoundWave`。  
4. **Acoustic Propagation System**：传播、衰减、延迟、反射/绕射（见第 4 点）。  
5. **Perception System**：耳朵采样叠加、加噪，写入 `Memory.spectral_buffer`。  
6. **Segmentation & Clustering System**：从连续声流**自发**分段、聚类形成原型（“音位”雏形）。  
7. **Meaning Inference System**（跨情境统计 + RL）：把听到的簇与当前情境/动作结果相关联，更新 `Lexicon`。  
8. **Imitation/Alignment System**：在成功交互后，向高回报个体的原型靠拢。  
9. **Energy & LifeCycle System**：代谢、成本结算、繁殖/死亡（选择压力驱动可发声/会听懂的个体占优）。  
10. **Logging/Telemetry System**：统一记录（见第 6 点）。

## 4）声波传递模型（两档可选）

**A. Packet/射线近似（实时友好，最小实现）**  
- 每个 `SoundWave` 存：`f0`、`duration`、`amp0`、`phase0`、`dir`。  
- 传播：位置 \(x\) 处的幅度 \(A(x)=A_0 / (1+\alpha d) \cdot e^{-\beta d}\)，到达时间 \(t=d/c\)。  
- 反射：与障碍法线做镜像射线；能量乘以反射系数 \(r\)。  
- 多源叠加：线性叠加→耳部限带滤波→SNR 门限。

**B. 栅格波场（更物理，但重）**  
- 在二维栅格上用简化波动方程离散：  
  \[
  u_{t+1}(x,y) = 2u_t - u_{t-1} + \lambda \nabla^2 u_t - \gamma u_t + s_t
  \]
- 障碍作为高阻抗单元，产生反射/绕射。  
- GPU 上用 Compute Shader 卷积/差分；对 `Ear` 做 STFT 采样形成时频图。

**建议**：先用 A（可跑大规模群体），验证后再切到 B 做高保真场景。

## 5）“语言”如何涌现（不设词典）

**分段（Segmentation）**  
- 在 `Perception` 后对能量包络和过零率做变化检测 → 得到候选片段。  

**聚类（Clustering）**  
- 将片段转为低维声学嵌入（MFCC/简化谱心+带宽+时长）。  
- 在线聚类（如 streaming k-means / DP-means）：形成**原型**（近似“音位/词形”）。  

**语义联结（Meaning Linking）**  
- 不预设“词义”，用**跨情境统计 + 强化**：  
  - 记录片段簇 \(S\) 与情境 \(C\)（如“我在这里”“有食物”“危险”“跟我来”）的共现计数与回报。  
  - 更新联结：  
    \[
    w_{S\to C} \leftarrow w_{S\to C} + \eta \cdot (r - \hat r)\cdot \mathbf{1}[S,C]
    \]
  - 说/听后的任务回报 \(r\) 高 → 该联结被强化。  

**对齐（Alignment）**  
- 成功互动后，听者将自己的某个原型向说者片段均值移动（Hebbian/模仿）：  
  \[
  \mu_{\text{hearer}} \leftarrow (1-\rho)\mu_{\text{hearer}} + \rho \cdot \text{embed}(S_{\text{speaker}})
  \]
- 群体内自然形成共享原型与“方言”。

**组合（可选进阶）**  
- 如果任务需要**序列**信息（比如“先 A 后 B”），允许个体产生多片段序列；  
- 统计序列转移矩阵 \(P(S_i\to S_j)\) 及其对回报的边际贡献，逐步涌现“原始语序”。

## 6）可观测与记录（你能看到什么）

**事件级日志（CSV/Parquet）**
- `emissions.csv`：`t, agent_id, pos, f0, duration, amp, seq_id`  
- `receptions.csv`：`t, agent_id, pos, source_id, snr, embed_vector`  
- `segments.csv`：`t, agent_id, seg_id, embed, cluster_id`  
- `links.csv`：`t, agent_id, cluster_id, context_id, Δw, reward`  
- `interactions.csv`：`t, speaker, listeners[], task_id, success(bool), reward_total`  
- `energy.csv`：`t, agent_id, energy, cost_vocal`  

**快照（JSON/Proto）**
- `lexicon_snapshot_{tick}.json`：每个体的 `prototypes`、`w_{S→C}`、`alignment_score`。  
- `population_metrics_{tick}.json`：  
  - 传递效率：**沟通成功率**、平均反应时  
  - **互信息** \(I(S;C)\)（信号与情境的 MI）  
  - **对齐度**：同一簇在群体中的方差/平均余弦相似度  
  - **性价比**：每单位发声能耗带来的回报

**可视化建议**
- 地图：声波等压线/射线动画 + 个体轨迹。  
- 语音：群体共享**原型云图**（t-SNE/UMAP 投影）随时间演化。  
- 指标：\(I(S;C)\)、成功率、能耗/回报曲线的时间序列。

## 7）最小规则集（Checklist，可直接开工）

**个体**
- 听：带噪接收，SNR 门限。  
- 说：连续参数化短声段；成本随振幅/时长线性增加。  
- 动：随机游走 + 目标驱动（资源、同伴）。  
- 学：  
  - 片段→嵌入→在线聚类；  
  - 簇—情境权重按回报更新；  
  - 成功后模仿说者原型（对齐）。  

**环境**
- 稀缺资源 + 协作阈值（必须≥2人触发/收益翻倍）。  
- 遮挡/反射障碍。  
- 掠食者/突发事件（喊叫有用）。

**物理**
- 声速 \(c\)，距离衰减 \(\alpha,\beta\)，噪声底。  
- 时间步长 \(\Delta t\)，最大发声时长 \(T_{\max}\)。

**决策**
- 发声门槛：\(\mathbb{E}[\Delta R] - \text{cost} > \tau\)。  
- 听后动作：按当前最大 \(w_{S\to C}\) 的情境去行动（探索率 \(\epsilon\)）。

## 8）系统伪代码（超简）

```pseudo
for tick in 1..T:
  Environment.Update()               # 资源/风险
  Movement.Foraging()
  for agent in Agents:
    if ExpectGain(agent) - VoiceCost(agent) > agent.threshold:
      s = agent.Vocalize()           # 生成 SoundWave
      World.Emit(s)
  Acoustic.Propagate()               # 衰减/延迟/反射
  for agent in Agents:
    Y = Ear.Sample(agent)            # 声学片段
    segs = Segment(Y)
    for seg in segs:
      z = Embed(seg)
      k = OnlineCluster(agent.lexicon, z)
      ctx = SenseContext(agent)      # 环境/任务态
      r = TaskRewardDelta(agent, ctx)
      UpdateLink(agent.lexicon, k, ctx, r)
      if InteractionSucceeded():
        AlignWithSpeaker(agent.lexicon, speaker.lexicon, k)
  EnergyLifeCycle.Update()
  Telemetry.LogAll()
```

## 9）三步落地路线

1) **MVP（2–3 周）**：射线近似 + 两人协作采集 + 4 个上下文（“我在这/有食物/危险/跟我来”）。验证 \(I(S;C)\) 从 0→上升。  
2) **对齐与方言**：多群体 + 障碍，观察不同群体的原型分化与接触区混合。  
3) **序列与语序**（可选）：让任务需“先 A 后 B”，观测两片段序列的稳定出现。

## 10）性能与工程提示
- ECS：将 `Perception/Segmentation/Clustering` 做批处理（struct-of-arrays），非常适合 GPU/Job System。  
- 声学嵌入用**极简特征**（能量、中心频、带宽、时长）即可跑通；后续再换 MFCC。  
- 记录 I/O：用按 tick 的分块文件，避免单文件膨胀；指标在后台（同一帧末）集中汇总。
