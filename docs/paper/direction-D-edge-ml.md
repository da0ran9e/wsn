# Direction D: Edge ML & Efficient Model Optimization

**Conference Track**: Track II - Signal Processing & Applications (Deep Learning, Edge AI)  
**Alignment**: ⭐⭐⭐ Strong  
**Complexity**: Medium  
**Implementation Timeline**: 5-7 weeks

---

## 1. Core Concept

**Problem Statement**: Current idea assumes CNN runs on RPi 4 with unspecified model. This direction systematically optimizes **lightweight ML models** for embedded person recognition on IoT devices.

**Focus Areas**:
1. **Model Efficiency**: Compare architectures (MobileNet vs SqueezeNet vs EfficientNet)
2. **Precision Reduction**: FP32 → INT8 quantization impact on accuracy
3. **Inference Latency**: Actual RPi 4 benchmarks (not theoretical)
4. **Fragment Utility**: Does feature-rich fragment help? Submodular analysis
5. **Adaptive Inference**: Trade accuracy vs speed in real-time

**Key Insight**: Lightweight CNN is mature (MobileNets published 2017), but **fragment-based person recognition** (partial views) is under-studied.

---

## 2. How This Affects the Core Idea

### 2.1 Detection Model Refinement

**Original Model**:
$$P(\text{target} | \mathcal{F}_i) = \frac{P(\mathcal{F}_i | \text{target}) \cdot P(\text{target})}{P(\mathcal{F}_i)}$$

Assumes $P(\mathcal{F}_i | \text{target})$ is known. **How to compute it?**

**Direction D Enhancement**:

```
Step 1: CNN Inference on Each Fragment
├─ Input: Fragment f_j (cropped image, e.g., 224×224)
├─ Model: MobileNetV2 (pre-trained on ImageNet)
├─ Output: Feature embedding (1280-d vector)
└─ Example: target person face → feature [0.3, 0.1, 0.9, ..., 0.2]

Step 2: Similarity Score
├─ Compare: target embedding vs fragment embedding
├─ Metric: Cosine similarity or L2 distance
├─ Score: sim(target, f_j) ∈ [0, 1]
└─ Interpretation: P(person in fragment matches target) ≈ sim score

Step 3: Fragment Utility via Submodular Maximization
├─ Question: Which fragments are informative?
│   └─ Face view (high-res, distinctive) >> background >> blur
├─ Marginal utility: U(f_j | {f_1, ..., f_{j-1}})
│   └─ How much does f_j improve confidence over already-held fragments?
├─ Property: Diminishing returns (face + face = less gain than face + body)
└─ Optimization: Greedy selection of high-utility fragments (98% optimal for submodular)

Step 4: Bayesian Fusion with Feature-Level Analysis
├─ Confidence update:
│   C_i(t) = P(target | {f_1, ..., f_k} received)
│          = softmax(weighted_sum(sim_1, sim_2, ..., sim_k))
│            where weights = [utility_1, utility_2, ...]
└─ Key change: Weights prioritize high-utility fragments
```

### 2.2 Inference Latency Budget

**Original**: Assumes ~100 ms for CNN inference (never validated)

**Direction D**: Measure actual latency on RPi 4

```
Latency Breakdown:
├─ Image preprocessing (resize, normalize): 5-10 ms
├─ CNN inference (forward pass):
│  ├─ MobileNetV2 (FP32): 100-150 ms
│  ├─ MobileNetV2 (INT8 quantized): 30-50 ms
│  ├─ SqueezeNet (FP32): 50-80 ms
│  └─ EfficientNet-Lite (INT8): 20-40 ms
├─ Feature extraction (post-processing): 5-10 ms
├─ Similarity computation: 1-2 ms
└─ Total: 41-212 ms depending on model
```

**Implication**: Inference latency ranges from 40-200 ms, not fixed 100 ms. This affects $T_{detect}$.

### 2.3 Accuracy vs Latency Trade-off

**Question**: Should node use fast model (40ms) or accurate model (200ms)?

**Scenario 1** (Early Detection Priority):
```
Use SqueezeNet INT8 (40 ms inference)
├─ Accuracy: ~75% (ImageNet top-1)
├─ Confidence threshold: θ = 0.8 (strict)
├─ Benefit: Fast feedback to BS
└─ Risk: False negatives (miss some matches)
```

**Scenario 2** (Accuracy Priority):
```
Use MobileNetV2 FP32 (150 ms inference)
├─ Accuracy: ~85% (ImageNet top-1)
├─ Confidence threshold: θ = 0.7
├─ Benefit: Higher recall (fewer misses)
└─ Risk: Slower detection (150 ms delay)
```

**Optimal**: Adaptive switching based on time pressure
```
Phase 1 (UAV recently arrived): Use fast model (40 ms)
├─ Rough screening, broadcast confidence beacons
└─ Accept lower accuracy

Phase 2 (UAV leaving soon, no detection yet): Switch to accurate model
├─ More careful processing
└─ Higher accuracy, slower response

Decision: Model switching at t = T_max * 0.7 (after 70% of mission time)
```

---

## 3. Implementation Approach

### 3.1 Phase 1: Model Comparison Benchmark (Weeks 1-2)

**Goal**: Measure actual inference latency & accuracy on RPi 4

**Models to Compare**:

| Model | Framework | FP32 Time | INT8 Time | Accuracy | Params | Size |
|-------|-----------|-----------|-----------|----------|--------|------|
| **MobileNetV2** | TF Lite | 150 ms | 50 ms | 85.0% | 3.5M | 14 MB |
| **SqueezeNet** | PyTorch | 80 ms | 30 ms | 75.4% | 1.2M | 5 MB |
| **EfficientNet-B0** | TF Lite | 250 ms | 70 ms | 86.7% | 5.3M | 21 MB |
| **MobileNetV3-Small** | TF Lite | 100 ms | 35 ms | 78.1% | 2.5M | 10 MB |

**Benchmark Script** (PyTorch):

```python
import torch
import torchvision.models as models
from time import perf_counter
import numpy as np

def benchmark_model(model_name, device='cpu', iterations=100):
  model = getattr(models, model_name)(pretrained=True)
  model.to(device)
  model.eval()
  
  # Dummy input
  input_tensor = torch.randn(1, 3, 224, 224).to(device)
  
  # Warm-up
  with torch.no_grad():
    for _ in range(10):
      _ = model(input_tensor)
  
  # Benchmark
  times = []
  with torch.no_grad():
    for _ in range(iterations):
      start = perf_counter()
      _ = model(input_tensor)
      times.append(perf_counter() - start)
  
  mean_time = np.mean(times[10:]) * 1000  # ms, skip warm-up
  std_time = np.std(times[10:]) * 1000
  
  print(f"{model_name}:")
  print(f"  Mean: {mean_time:.1f} ms ± {std_time:.1f} ms")
  return mean_time

# Run on RPi 4
if __name__ == '__main__':
  models_to_test = ['mobilenet_v2', 'squeezenet1_0', 'efficientnet_b0']
  
  for model_name in models_to_test:
    benchmark_model(model_name)
```

**Expected Outcomes**:
- MobileNetV2: Best balance (85% accuracy, 50-150 ms latency)
- SqueezeNet: Fastest (~30 ms INT8), lower accuracy
- EfficientNet: Highest accuracy, slower

**Recommendation**: Use MobileNetV2 as primary, SqueezeNet as fallback (time-constrained)

### 3.2 Phase 2: Quantization & Pruning (Weeks 1-2, parallel)

**Goal**: Reduce model size & latency via compression

**Quantization (FP32 → INT8)**:

```python
import tensorflow as tf

def quantize_model(model_fp32):
  """
  Convert FP32 model to INT8 using post-training quantization
  """
  converter = tf.lite.TFLiteConverter.from_keras_model(model_fp32)
  converter.optimizations = [tf.lite.Optimize.DEFAULT]
  
  # Quantization-aware training (better accuracy retention)
  # or post-training quantization (simpler, acceptable for ~1-3% accuracy drop)
  
  quantized_model = converter.convert()
  return quantized_model
```

**Expected Impact**:
```
MobileNetV2 Quantization:
├─ Model size: 14 MB (FP32) → 3.5 MB (INT8), 75% reduction
├─ Inference time: 150 ms (FP32) → 50 ms (INT8), 3x speedup
├─ Accuracy: 85.0% (FP32) → 84.3% (INT8), 0.7% loss (acceptable)
└─ Memory: 14 MB loaded → 3.5 MB, fits easily on RPi 4 (4GB RAM)
```

**Pruning (Remove unimportant weights)**:

```python
def prune_model(model, sparsity=0.5):
  """
  Remove 50% of weights (set to zero)
  Requires retraining for accuracy recovery
  """
  pruning_params = {
    'pruning_schedule': tf.keras.optimizers.schedules.PolynomialDecay(...)
  }
  
  model = tfmot.sparsity.keras.prune_low_magnitude(model, **pruning_params)
  # Retrain on person recognition dataset
  
  stripped_model = tfmot.sparsity.keras.strip_pruning(model)
  return stripped_model
```

**Trade-off**: Pruning requires retraining (expensive), but 30-40% latency improvement possible

**Recommendation for timeline**: Skip pruning, focus on quantization (quicker, good enough)

### 3.3 Phase 3: Fragment-Based Person Recognition Dataset (Weeks 2-3)

**Goal**: Evaluate models on partial image views (key difference from standard ImageNet)

**Dataset Construction**:

```python
def create_fragment_dataset():
  """
  Take full images, create fragmented versions
  """
  full_images = load_coco_persons()  # e.g., COCO person annotations
  
  fragments = {
    'face': [],       # Crop face region (100×100)
    'upper_body': [], # Crop torso (150×150)
    'full': [],       # Full body (224×224)
    'clothing': [],   # Lower body (100×100)
    'blurry': []      # Gaussian blur (simulate UAV motion blur)
  }
  
  for img in full_images:
    # Face fragment
    face_box = detect_face(img)
    fragments['face'].append(crop(img, face_box))
    
    # Clothing fragment
    body_box = detect_body(img)
    fragments['clothing'].append(crop_lower(img, body_box))
    
    # Blurry (simulate UAV motion)
    fragments['blurry'].append(gaussian_blur(img, sigma=2))
  
  return fragments

# Evaluation
def evaluate_fragment_accuracy():
  model = load_pretrained_mobilenet()
  
  results = {}
  for fragment_type in ['face', 'upper_body', 'full', 'clothing', 'blurry']:
    frag_images = fragments[fragment_type]
    
    accuracy = measure_accuracy(model, frag_images)
    results[fragment_type] = accuracy
  
  # Expected: face >> full >> upper_body >> clothing >> blurry
  return results
```

**Expected Results** (person re-identification, not ImageNet classification):

| Fragment Type | Accuracy | Utility |
|---|---|---|
| **Face** | 95% | High (most discriminative) |
| **Full body** | 80% | Medium (complete view) |
| **Upper body** | 65% | Medium (clothing variation) |
| **Lower body** | 50% | Low (generic clothing) |
| **Blurry** | 35% | Very Low (quality degraded) |

**Key Finding**: Face fragments ~3x more informative than lower body

### 3.4 Phase 4: Fragment Utility & Submodular Maximization (Weeks 3-4)

**Goal**: Formalize which fragments contribute most to detection

**Submodular Function Definition**:

$$U(f_j | F) = C(\mathcal{F} \cup \{f_j\}) - C(\mathcal{F})$$

Where:
- $C(\mathcal{F})$ = confidence from fragment set $\mathcal{F}$
- $U(f_j | F)$ = marginal utility of adding $f_j$

**Submodular Property**: $U(f_j | F) \geq U(f_j | F')$ if $F \subseteq F'$
(utility decreases as more fragments already collected)

**Example**:
```
Start: C({}) = 0.1 (no fragments, random guess)
├─ Add face (f_1): C({f_1}) = 0.85, U(f_1|{}) = 0.75 (high utility)
├─ Add clothing (f_2): C({f_1, f_2}) = 0.88, U(f_2|{f_1}) = 0.03 (low utility, marginal)
├─ [Alternative] Add upper body (f_3): C({f_1, f_3}) = 0.87, U(f_3|{f_1}) = 0.02 (similar low)
└─ Add blurry (f_4): C({f_1, f_2, f_3, f_4}) = 0.88, U(f_4|{f_1,f_2,f_3}) = 0.00 (negligible)
```

**Greedy Maximization**:

```python
def greedy_fragment_selection(all_fragments, current_confidence=0.5, target=0.9):
  """
  Greedily select high-utility fragments until confidence target met
  """
  selected = []
  confidence = current_confidence
  
  while confidence < target and len(selected) < len(all_fragments):
    # Compute marginal utility of each remaining fragment
    best_fragment = None
    best_utility = -1
    
    for frag in all_fragments:
      if frag in selected:
        continue
      
      # Tentatively add fragment
      test_set = selected + [frag]
      new_confidence = evaluate_confidence(test_set)
      utility = new_confidence - confidence
      
      if utility > best_utility:
        best_utility = utility
        best_fragment = frag
    
    if best_fragment is None or best_utility < threshold_utility:
      break  # No more useful fragments
    
    selected.append(best_fragment)
    confidence += best_utility
  
  return selected, confidence

# Use in simulation
for target_person in targets:
  fragments_received = []
  
  for f in uav_broadcast_schedule:
    fragments_received.append(f)
    
    selected = greedy_fragment_selection(
      all_fragments=fragments_received,
      current_confidence=node_confidence,
      target=threshold_high
    )
    
    if node_confidence > threshold_high:
      ALERT()  # Early termination
```

**Simulation Comparison**:
- **Uniform order** (transmit in order 1, 2, 3, ...): Needs all fragments
- **Greedy utility order** (transmit high-utility first): Faster detection
- **Optimal order** (solve MSB problem): Theoretical upper bound

**Expected**: Greedy order 20-30% faster than uniform

### 3.5 Phase 5: Adaptive Inference Policy (Weeks 4-5)

**Goal**: Switch model based on mission time pressure

**Policy Design**:

```python
class AdaptiveInferenceController:
  def __init__(self, mission_time_s=10):
    self.start_time = time.now()
    self.mission_time = mission_time_s
    self.fast_model = load_squeezenet_int8()  # 30 ms
    self.accurate_model = load_mobilenet_fp32()  # 150 ms
    
  def select_model(self):
    """
    Switch model based on time remaining
    """
    elapsed = time.now() - self.start_time
    fraction_remaining = 1.0 - (elapsed / self.mission_time)
    
    if fraction_remaining > 0.7:
      # Early mission: fast screening
      return self.fast_model
    elif fraction_remaining > 0.3:
      # Mid mission: balanced
      return self.fast_model if random() < 0.5 else self.accurate_model
    else:
      # Late mission: last chance, use accurate
      return self.accurate_model
  
  def infer(self, fragment_image):
    model = self.select_model()
    embedding = model(fragment_image)
    confidence = similarity(embedding, target_embedding)
    return confidence
```

**Expected Benefit**:
```
Fixed fast model:  T_detect = 600 ms (misses some detections)
Fixed accurate:    T_detect = 400 ms (slow but reliable)
Adaptive:          T_detect = 350 ms (fast early + accurate late)
Improvement:       ~15-20% vs best fixed strategy
```

### 3.6 Phase 6: Integration with Simulation (Weeks 5-6)

**Goal**: Use empirical inference model in ns-3 simulation

```cpp
// ns-3 callback for node inference
class EdgeInferenceModule {
  float infer(Fragment f) {
    // Use empirical timing measurements
    ModelType model = SelectModel(current_time);  // Adaptive
    
    // Simulate inference time (from Phase 1 benchmarks)
    Time inference_time;
    if (model == SQUEEZENET_INT8) {
      inference_time = Milliseconds(30);
      accuracy = 0.75;
    } else {
      inference_time = Milliseconds(150);
      accuracy = 0.85;
    }
    
    // Simulate inference result (stochastic based on accuracy)
    float similarity = random_normal(mean=accuracy, std=0.05);
    similarity = clip(similarity, 0.0, 1.0);
    
    Simulator::Schedule(inference_time, &UpdateConfidence, similarity);
    
    return similarity;
  }
};
```

**Simulation Parameters from Benchmarks**:
- SqueezeNet INT8: 30 ms, 75% accuracy
- MobileNetV2 INT8: 50 ms, 84% accuracy
- MobileNetV2 FP32: 150 ms, 85% accuracy

**Scenarios**:
- Baseline: All nodes use MobileNetV2 FP32
- Fast: All nodes use SqueezeNet INT8
- Adaptive: Nodes switch per phase

---

## 4. Detailed Analysis by Component

### 4.1 Model Selection Trade-off

**Question**: Which model to deploy on 50 nodes?

**Factors**:
1. **Cost**: Model size × deployment cost ($per model)
2. **Energy**: Inference power × frequency (battery lifetime)
3. **Accuracy**: Person recognition accuracy (safety-critical)
4. **Latency**: Time-to-first-detection

**Decision Matrix**:

| Model | Size | Energy/inference | Accuracy | Latency | Cost Index |
|---|---|---|---|---|---|
| SqueezeNet INT8 | 3 MB | 0.1 J | 75% | 30 ms | 1.0 |
| MobileNetV2 INT8 | 3.5 MB | 0.15 J | 84% | 50 ms | 1.1 |
| MobileNetV2 FP32 | 14 MB | 0.50 J | 85% | 150 ms | 1.5 |
| EfficientNet INT8 | 8 MB | 0.25 J | 86% | 70 ms | 1.3 |

**Recommendation for AC-powered urban nodes**:
- **Primary**: MobileNetV2 FP32 (best accuracy, energy not constraint)
- **Fallback**: MobileNetV2 INT8 (balanced)
- **Time-critical**: SqueezeNet INT8 (fastest)

### 4.2 Feature Extraction & Embedding Space

**Key Question**: Do all layers contribute equally?

**Analysis**:
```
MobileNetV2 Architecture:
├─ Input: 224×224×3
├─ Layer 1-4: Low-level features (edges, textures)
│  └─ Contribute to general image understanding
├─ Layer 5-10: Mid-level (object parts)
│  └─ Contribute to body/clothing recognition
├─ Layer 11-16: High-level (semantic concepts)
│  └─ Contribute to person identity
└─ Final layer (embeddings): 1280-d vector
   └─ Can be pruned back to 256-d with minimal accuracy loss
```

**Dimensionality Reduction**:
```
1280-d embedding → too large for low-bandwidth transmission
├─ Option A: Use as-is (14 KB per node per time)
├─ Option B: PCA to 128-d (1 KB per node, 95% variance retained)
└─ Option C: Hash to binary (128 bits, 16 B, very fast comparison)
```

**For cooperative detection across nodes**:
- Node A embedding: 1280-d
- Node B requests embedding from A: Use PCA-128d (1 KB transmission)
- Loss: ~5% accuracy, gain: 14x bandwidth reduction

### 4.3 Fragment-Quality Degradation

**Real World**: UAV-captured fragments subject to:
- Motion blur (UAV velocity)
- Low resolution (altitude 100m)
- Compression artifacts (transmission)
- Varying lighting

**Model Robustness**:

```python
def evaluate_robustness(model):
  """
  Test model accuracy under degradations
  """
  clean_images = load_test_set()
  
  results = {
    'clean': [],
    'blur_sigma1': [],
    'blur_sigma2': [],
    'noise_snr20': [],
    'compression_q90': [],
    'compression_q50': []
  }
  
  for condition in results:
    degraded = apply_degradation(clean_images, condition)
    accuracy = evaluate(model, degraded)
    results[condition] = accuracy
  
  return results

# Expected (MobileNetV2):
# Clean: 85%
# Motion blur (σ=1): 82% (3% drop)
# Motion blur (σ=2): 75% (10% drop) ← significant
# JPEG compression (Q=90): 83% (2% drop)
# JPEG compression (Q=50): 70% (15% drop) ← significant
```

**Implications**:
1. Model not robust to severe blur → affects higher altitudes
2. Compression hurts significantly → transmit fragments before compression
3. Recommendation: Transmit raw/lightly-compressed fragments

### 4.4 Cooperative Model Refinement

**Idea**: Nodes with good local compute can help others

**Scenario**:
```
Node A (high-compute, multi-GPU): Runs full MobileNetV2 FP32
Node B (low-compute, single-CPU): Runs SqueezeNet INT8 only

Node B receives fragment f_j, gets confidence 0.65 (borderline)
├─ Option 1: Use 0.65 locally, request from Node A
├─ Option 2: Send raw fragment to A, get A's confidence (0.82)
├─ Option 3: Send A only the relevant part (e.g., face crop)

Workflow:
1. B broadcasts: "Fragment f_j, my confidence 0.65, computing..."
2. A receives B's signal → also processes f_j → gets 0.82
3. A broadcasts: "f_j confidence 0.82 from me"
4. B integrates (e.g., average: 0.735, or trust A: 0.82)
5. New confidence > threshold → ALERT

Benefit: Cooperative inference improves accuracy
Cost: +transmission overhead
```

**Formal Model**:
$$C_B = \alpha C_B^{SqueezeNet} + (1-\alpha) C_A^{MobileNet}$$

Where $\alpha$ = trust weight (favor local for speed, trust A for accuracy)

---

## 5. Integration with Current Idea.md

**Modifications**:

1. **Section on IoT Nodes - AI Model**:
   - Expand with specific model recommendations (MobileNetV2)
   - Add inference latency measurements
   - Quantization benefits (+50% speedup for <1% accuracy loss)

2. **Detection Model Section**:
   - Clarify: $P(f_j | \text{target}, \text{node}_i)$ = CNN similarity score
   - Add submodular utility formulation
   - Explain greedy fragment selection

3. **Fragments Section**:
   - Add fragment types (face, upper body, full, blurry)
   - Utility table (face > body > clothing)

4. **New Subsection**: "Edge ML & Fragment Recognition"
   - Model benchmarks table
   - Adaptive inference policy
   - Fragment-based accuracy analysis

---

## 6. Expected Contributions

### 6.1 Scientific Novelty
- First systematic study of CNN models for UAV-captured fragments
- Submodular analysis of fragment utility (novel for person recognition)
- Adaptive inference policy (switching models per time pressure)
- Cooperative refinement across heterogeneous compute

### 6.2 Experimental Insights
- MobileNetV2 FP32 best overall (85% accuracy, balanced latency)
- Fragment-based accuracy hierarchy (face >> body)
- Greedy selection 20-30% faster than uniform order
- Adaptive policy 15-20% improvement over fixed strategy

### 6.3 Practical Contribution
- RPi 4 benchmarks (reproducible on commodity hardware)
- TensorFlow Lite recipes for deployment
- Fragment dataset creation methodology

---

## 7. Simulation Scenarios

### Scenario 1: Model Comparison
```
Compare: SqueezeNet vs MobileNetV2 vs EfficientNet
Metric: T_detect, accuracy, inference latency
Expected: MobileNetV2 wins overall
```

### Scenario 2: Quantization Impact
```
FP32 vs INT8 for MobileNetV2
Expected: INT8 3x faster, 0.7% accuracy drop (net positive)
```

### Scenario 3: Fragment Utility
```
Random order vs greedy utility order for fragment transmission
Expected: Greedy 20-30% faster detection
```

### Scenario 4: Adaptive Policy
```
Fixed model vs adaptive (switch per time pressure)
Expected: Adaptive 15-20% faster
```

### Scenario 5: Cooperative Refinement
```
Single node vs multi-node inference sharing
Expected: Accuracy +3-5%, latency -10-20 ms
```

---

## 8. Open Questions

1. **Transfer Learning**: Does pre-trained ImageNet model work for person re-ID? (or need specialized training?)

2. **Blur Robustness**: How high can altitude be before blur degrades accuracy? (altitude vs model robustness trade-off)

3. **Online Learning**: Can nodes adapt their models during mission? (federated learning on-the-fly)

4. **Synthetic Data**: Can simulated UAV footage train models effectively? (data augmentation)

5. **Hardware Acceleration**: Can RPi 4 use TPU/GPU for inference? (hardware-specific optimization)

---

## 9. Timeline & Effort Estimate

| Phase | Duration | Effort | Deliverable |
|-------|----------|--------|-------------|
| Model benchmarking | 1 week | High | Latency measurements, accuracy comparison |
| Quantization | 1 week | Medium | INT8 conversion, loss analysis |
| Fragment dataset | 1 week | High | COCO-based fragment creation |
| Submodular analysis | 1 week | Medium | Utility model, greedy optimization |
| Adaptive inference | 1 week | Low-Med | Policy implementation, simulation |
| Integration & scenarios | 1 week | Medium | ns-3 integration, 5 scenarios |
| Paper writing | 1 week | High | 12-16 pages |

**Total**: 5-7 weeks (shorter than A/B/C due to less simulation complexity)

**Minimum for publication**: Phases 1-3 (3 weeks) = benchmarks + quantization + fragments

---
