/*
 * 3D Sorting Algorithm Visualization using Raylib (C++)
 * Built with Raylib 6.0
 */

#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <cmath>
#include <string>
#include <algorithm>
#include <random>
#include <chrono>

// ============================================================================
// Data Structures & Constants
// ============================================================================

enum AlgorithmType {
    ALG_BUBBLE = 0,
    ALG_SELECTION,
    ALG_INSERTION,
    ALG_QUICK,
    ALG_MERGE,
    ALG_HEAP,
    ALG_COUNT
};

const char* ALG_NAMES[ALG_COUNT] = {
    "Bubble Sort",
    "Selection Sort",
    "Insertion Sort",
    "Quick Sort",
    "Merge Sort",
    "Heap Sort"
};

enum DistributionType {
    DIST_RANDOM = 0,
    DIST_REVERSED,
    DIST_NEARLY_SORTED,
    DIST_FEW_UNIQUE,
    DIST_COUNT
};

const char* DIST_NAMES[DIST_COUNT] = {
    "Random",
    "Reversed",
    "Nearly Sorted",
    "Few Unique"
};

enum StepType {
    STEP_NONE = 0,
    STEP_COMPARE,
    STEP_SWAP,
    STEP_SET,
    STEP_PIVOT,
    STEP_MARK_SORTED
};

struct SortStep {
    StepType type;
    int indexA;
    int indexB;
    int valueA;
    int valueB;
};

struct Bar {
    int value;               // Original value (1..N)
    Vector3 currentPos;      // Current 3D position
    Vector3 targetPos;       // Target 3D position for sliding/swapping
    float arcProgress;       // 0.0 to 1.0 for parabolic swap arc
    float arcHeight;         // Height of swap arc jump
    Color color;             // Base color (gradient)
    float highlightTimer;    // Highlight animation glow timer
    Color highlightColor;    // Temporary highlight color
    bool isSorted;           // Whether element is in final sorted position
    float bounceScale;       // Scale multiplier for bounce effect
};

struct Particle {
    Vector3 position;
    Vector3 velocity;
    Color color;
    float size;
    float life;              // Remaining life in seconds
    float maxLife;
};

// ============================================================================
// Global State & Visualizer Class
// ============================================================================

class Visualizer {
public:
    int elementCount = 30;
    std::vector<int> rawArray;
    std::vector<Bar> bars;
    std::vector<SortStep> steps;
    size_t currentStepIndex = 0;

    AlgorithmType currentAlg = ALG_BUBBLE;
    DistributionType currentDist = DIST_RANDOM;

    bool isPlaying = false;
    bool isFinished = false;
    float stepTimer = 0.0f;
    float speedMultiplier = 15.0f; // Steps per second

    int comparisonCount = 0;
    int swapCount = 0;
    float elapsedTime = 0.0f;

    // Active indices for highlighting
    int activeA = -1;
    int activeB = -1;
    int pivotIndex = -1;

    // Particles
    std::vector<Particle> particles;

    // Audio
    std::vector<Sound> pitchSounds;
    bool soundEnabled = true;

    // Camera
    Camera3D camera;
    float cameraAngle = 0.0f;
    bool autoRotateCamera = false;

    // Dimensions
    const float barWidth = 0.8f;
    const float barSpacing = 1.1f;
    const float maxHeight = 12.0f;

    Visualizer() {
        InitCamera();
    }

    void InitCamera() {
        camera.position = (Vector3){ 0.0f, 18.0f, 32.0f };
        camera.target = (Vector3){ 0.0f, 4.0f, 0.0f };
        camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
        camera.fovy = 45.0f;
        camera.projection = CAMERA_PERSPECTIVE;
    }

    void InitAudio() {
        InitAudioDevice();
        // Generate a scale of synthesized sounds (32 pitches)
        const int numPitches = 32;
        pitchSounds.resize(numPitches);

        int sampleRate = 22050;
        float duration = 0.08f;
        int sampleCount = (int)(sampleRate * duration);

        for (int p = 0; p < numPitches; p++) {
            // Frequency range from ~180Hz to ~1200Hz
            float freq = 180.0f * powf(2.0f, (float)p / 8.0f);
            short* samples = (short*)RL_MALLOC(sampleCount * sizeof(short));

            for (int i = 0; i < sampleCount; i++) {
                float t = (float)i / sampleRate;
                // Envelope (fade in / exponential fade out)
                float env = expf(-t * 25.0f);
                float wave = sinf(2.0f * PI * freq * t);
                // Add soft sub-harmonic for warmth
                wave += 0.3f * sinf(PI * freq * t);
                samples[i] = (short)(wave * env * 12000.0f);
            }

            Wave wave = { 0 };
            wave.frameCount = sampleCount;
            wave.sampleRate = sampleRate;
            wave.sampleSize = 16;
            wave.channels = 1;
            wave.data = samples;

            pitchSounds[p] = LoadSoundFromWave(wave);
            UnloadWave(wave);
        }
    }

    void CleanAudio() {
        for (auto& sound : pitchSounds) {
            UnloadSound(sound);
        }
        CloseAudioDevice();
    }

    void PlayPitchSound(int val) {
        if (!soundEnabled || pitchSounds.empty()) return;
        int pIndex = (int)(((float)val / (float)elementCount) * (pitchSounds.size() - 1));
        pIndex = Clamp(pIndex, 0, (int)pitchSounds.size() - 1);
        PlaySound(pitchSounds[pIndex]);
    }

    Color GetHSVColor(float valueNormalized) {
        // Map 0..1 to vibrant HSV color spectrum (Blue -> Cyan -> Green -> Yellow -> Red -> Magenta)
        float hue = (1.0f - valueNormalized) * 240.0f; // 240=Blue to 0=Red
        return ColorFromHSV(hue, 0.85f, 0.95f);
    }

    void ResetArray() {
        isPlaying = false;
        isFinished = false;
        currentStepIndex = 0;
        comparisonCount = 0;
        swapCount = 0;
        elapsedTime = 0.0f;
        activeA = -1;
        activeB = -1;
        pivotIndex = -1;
        particles.clear();

        rawArray.resize(elementCount);
        for (int i = 0; i < elementCount; i++) {
            rawArray[i] = i + 1;
        }

        // Apply Distribution
        std::mt19937 rng(1337 + (int)time(NULL));
        if (currentDist == DIST_RANDOM) {
            std::shuffle(rawArray.begin(), rawArray.end(), rng);
        } else if (currentDist == DIST_REVERSED) {
            std::reverse(rawArray.begin(), rawArray.end());
        } else if (currentDist == DIST_NEARLY_SORTED) {
            std::sort(rawArray.begin(), rawArray.end());
            for (int k = 0; k < elementCount / 5; k++) {
                int idx1 = rng() % elementCount;
                int idx2 = rng() % elementCount;
                std::swap(rawArray[idx1], rawArray[idx2]);
            }
        } else if (currentDist == DIST_FEW_UNIQUE) {
            int numBuckets = 4;
            for (int i = 0; i < elementCount; i++) {
                int bucket = (i * numBuckets) / elementCount;
                rawArray[i] = (bucket + 1) * (elementCount / numBuckets);
            }
            std::shuffle(rawArray.begin(), rawArray.end(), rng);
        }

        // Initialize 3D Bars
        bars.resize(elementCount);
        float totalWidth = elementCount * barSpacing;
        float startX = -totalWidth / 2.0f + barSpacing / 2.0f;

        for (int i = 0; i < elementCount; i++) {
            int val = rawArray[i];
            float normVal = (float)val / (float)elementCount;
            float h = normVal * maxHeight + 0.5f;

            Vector3 pos = { startX + i * barSpacing, h / 2.0f, 0.0f };
            bars[i].value = val;
            bars[i].currentPos = pos;
            bars[i].targetPos = pos;
            bars[i].arcProgress = 0.0f;
            bars[i].arcHeight = 0.0f;
            bars[i].color = GetHSVColor(normVal);
            bars[i].highlightTimer = 0.0f;
            bars[i].highlightColor = WHITE;
            bars[i].isSorted = false;
            bars[i].bounceScale = 1.0f;
        }

        GenerateSteps();
    }

    // ========================================================================
    // Step Generation Algorithms
    // ========================================================================

    void GenerateSteps() {
        steps.clear();
        std::vector<int> arr = rawArray;

        switch (currentAlg) {
            case ALG_BUBBLE: GenerateBubbleSteps(arr); break;
            case ALG_SELECTION: GenerateSelectionSteps(arr); break;
            case ALG_INSERTION: GenerateInsertionSteps(arr); break;
            case ALG_QUICK: GenerateQuickSteps(arr, 0, (int)arr.size() - 1); break;
            case ALG_MERGE: GenerateMergeSteps(arr, 0, (int)arr.size() - 1); break;
            case ALG_HEAP: GenerateHeapSteps(arr); break;
            default: break;
        }

        // Finalize: mark all sorted
        for (int i = 0; i < (int)arr.size(); i++) {
            SortStep s;
            s.type = STEP_MARK_SORTED;
            s.indexA = i;
            s.indexB = -1;
            s.valueA = arr[i];
            s.valueB = -1;
            steps.push_back(s);
        }
    }

    void GenerateBubbleSteps(std::vector<int>& arr) {
        int n = arr.size();
        for (int i = 0; i < n - 1; i++) {
            for (int j = 0; j < n - i - 1; j++) {
                steps.push_back({ STEP_COMPARE, j, j + 1, arr[j], arr[j + 1] });
                if (arr[j] > arr[j + 1]) {
                    std::swap(arr[j], arr[j + 1]);
                    steps.push_back({ STEP_SWAP, j, j + 1, arr[j], arr[j + 1] });
                }
            }
            steps.push_back({ STEP_MARK_SORTED, n - 1 - i, -1, arr[n - 1 - i], -1 });
        }
    }

    void GenerateSelectionSteps(std::vector<int>& arr) {
        int n = arr.size();
        for (int i = 0; i < n - 1; i++) {
            int minIdx = i;
            steps.push_back({ STEP_PIVOT, minIdx, -1, arr[minIdx], -1 });
            for (int j = i + 1; j < n; j++) {
                steps.push_back({ STEP_COMPARE, minIdx, j, arr[minIdx], arr[j] });
                if (arr[j] < arr[minIdx]) {
                    minIdx = j;
                    steps.push_back({ STEP_PIVOT, minIdx, -1, arr[minIdx], -1 });
                }
            }
            if (minIdx != i) {
                std::swap(arr[i], arr[minIdx]);
                steps.push_back({ STEP_SWAP, i, minIdx, arr[i], arr[minIdx] });
            }
            steps.push_back({ STEP_MARK_SORTED, i, -1, arr[i], -1 });
        }
    }

    void GenerateInsertionSteps(std::vector<int>& arr) {
        int n = arr.size();
        for (int i = 1; i < n; i++) {
            int key = arr[i];
            int j = i - 1;
            steps.push_back({ STEP_PIVOT, i, -1, key, -1 });
            while (j >= 0) {
                steps.push_back({ STEP_COMPARE, j, j + 1, arr[j], key });
                if (arr[j] > key) {
                    arr[j + 1] = arr[j];
                    steps.push_back({ STEP_SWAP, j, j + 1, arr[j + 1], arr[j] });
                    j--;
                } else {
                    break;
                }
            }
            arr[j + 1] = key;
            steps.push_back({ STEP_SET, j + 1, -1, key, -1 });
        }
    }

    void GenerateQuickSteps(std::vector<int>& arr, int low, int high) {
        if (low < high) {
            int pivotVal = arr[high];
            steps.push_back({ STEP_PIVOT, high, -1, pivotVal, -1 });
            int i = low - 1;
            for (int j = low; j < high; j++) {
                steps.push_back({ STEP_COMPARE, j, high, arr[j], pivotVal });
                if (arr[j] < pivotVal) {
                    i++;
                    std::swap(arr[i], arr[j]);
                    steps.push_back({ STEP_SWAP, i, j, arr[i], arr[j] });
                }
            }
            std::swap(arr[i + 1], arr[high]);
            steps.push_back({ STEP_SWAP, i + 1, high, arr[i + 1], arr[high] });
            steps.push_back({ STEP_MARK_SORTED, i + 1, -1, arr[i + 1], -1 });

            int pi = i + 1;
            GenerateQuickSteps(arr, low, pi - 1);
            GenerateQuickSteps(arr, pi + 1, high);
        } else if (low == high) {
            steps.push_back({ STEP_MARK_SORTED, low, -1, arr[low], -1 });
        }
    }

    void GenerateMergeSteps(std::vector<int>& arr, int l, int r) {
        if (l >= r) return;
        int m = l + (r - l) / 2;
        GenerateMergeSteps(arr, l, m);
        GenerateMergeSteps(arr, m + 1, r);

        // Merge logic
        std::vector<int> left(arr.begin() + l, arr.begin() + m + 1);
        std::vector<int> right(arr.begin() + m + 1, arr.begin() + r + 1);

        int i = 0, j = 0, k = l;
        while (i < (int)left.size() && j < (int)right.size()) {
            steps.push_back({ STEP_COMPARE, l + i, m + 1 + j, left[i], right[j] });
            if (left[i] <= right[j]) {
                arr[k] = left[i];
                steps.push_back({ STEP_SET, k, -1, left[i], -1 });
                i++;
            } else {
                arr[k] = right[j];
                steps.push_back({ STEP_SET, k, -1, right[j], -1 });
                j++;
            }
            k++;
        }
        while (i < (int)left.size()) {
            arr[k] = left[i];
            steps.push_back({ STEP_SET, k, -1, left[i], -1 });
            i++;
            k++;
        }
        while (j < (int)right.size()) {
            arr[k] = right[j];
            steps.push_back({ STEP_SET, k, -1, right[j], -1 });
            j++;
            k++;
        }
    }

    void Heapify(std::vector<int>& arr, int n, int i) {
        int largest = i;
        int left = 2 * i + 1;
        int right = 2 * i + 2;

        if (left < n) {
            steps.push_back({ STEP_COMPARE, left, largest, arr[left], arr[largest] });
            if (arr[left] > arr[largest]) largest = left;
        }
        if (right < n) {
            steps.push_back({ STEP_COMPARE, right, largest, arr[right], arr[largest] });
            if (arr[right] > arr[largest]) largest = right;
        }

        if (largest != i) {
            std::swap(arr[i], arr[largest]);
            steps.push_back({ STEP_SWAP, i, largest, arr[i], arr[largest] });
            Heapify(arr, n, largest);
        }
    }

    void GenerateHeapSteps(std::vector<int>& arr) {
        int n = arr.size();
        for (int i = n / 2 - 1; i >= 0; i--) {
            Heapify(arr, n, i);
        }
        for (int i = n - 1; i > 0; i--) {
            std::swap(arr[0], arr[i]);
            steps.push_back({ STEP_SWAP, 0, i, arr[0], arr[i] });
            steps.push_back({ STEP_MARK_SORTED, i, -1, arr[i], -1 });
            Heapify(arr, i, 0);
        }
    }

    // ========================================================================
    // Execution & Animation Updates
    // ========================================================================

    void SpawnSwapParticles(Vector3 pos1, Vector3 pos2) {
        Vector3 mid = Vector3Scale(Vector3Add(pos1, pos2), 0.5f);
        for (int i = 0; i < 15; i++) {
            Particle p;
            p.position = mid;
            p.velocity = (Vector3){
                (float)(rand() % 100 - 50) / 10.0f,
                (float)(rand() % 100) / 8.0f + 2.0f,
                (float)(rand() % 100 - 50) / 10.0f
            };
            p.color = (rand() % 2 == 0) ? YELLOW : ORANGE;
            p.size = (float)(rand() % 10 + 5) / 20.0f;
            p.maxLife = (float)(rand() % 50 + 30) / 100.0f;
            p.life = p.maxLife;
            particles.push_back(p);
        }
    }

    void SpawnSortedParticles(Vector3 pos) {
        for (int i = 0; i < 8; i++) {
            Particle p;
            p.position = pos;
            p.velocity = (Vector3){
                (float)(rand() % 100 - 50) / 15.0f,
                (float)(rand() % 100) / 10.0f + 1.0f,
                (float)(rand() % 100 - 50) / 15.0f
            };
            p.color = LIME;
            p.size = 0.25f;
            p.maxLife = 0.4f;
            p.life = p.maxLife;
            particles.push_back(p);
        }
    }

    void ExecuteNextStep() {
        if (currentStepIndex >= steps.size()) {
            isPlaying = false;
            isFinished = true;
            activeA = -1;
            activeB = -1;
            pivotIndex = -1;
            return;
        }

        SortStep step = steps[currentStepIndex];
        currentStepIndex++;

        switch (step.type) {
            case STEP_COMPARE: {
                comparisonCount++;
                activeA = step.indexA;
                activeB = step.indexB;
                if (activeA >= 0) {
                    bars[activeA].highlightTimer = 0.2f;
                    bars[activeA].highlightColor = YELLOW;
                    PlayPitchSound(bars[activeA].value);
                }
                if (activeB >= 0) {
                    bars[activeB].highlightTimer = 0.2f;
                    bars[activeB].highlightColor = YELLOW;
                }
                break;
            }
            case STEP_SWAP: {
                swapCount++;
                int idxA = step.indexA;
                int idxB = step.indexB;
                activeA = idxA;
                activeB = idxB;

                // Swap data in bars vector
                Bar tempBar = bars[idxA];
                bars[idxA] = bars[idxB];
                bars[idxB] = tempBar;

                // Set targets for smooth sliding & arc bounce
                float totalWidth = elementCount * barSpacing;
                float startX = -totalWidth / 2.0f + barSpacing / 2.0f;

                Vector3 targetPosA = { startX + idxA * barSpacing, bars[idxA].currentPos.y, 0.0f };
                Vector3 targetPosB = { startX + idxB * barSpacing, bars[idxB].currentPos.y, 0.0f };

                bars[idxA].targetPos = targetPosA;
                bars[idxA].arcProgress = 0.0f;
                bars[idxA].arcHeight = 2.5f;
                bars[idxA].highlightTimer = 0.3f;
                bars[idxA].highlightColor = RED;
                bars[idxA].bounceScale = 1.25f;

                bars[idxB].targetPos = targetPosB;
                bars[idxB].arcProgress = 0.0f;
                bars[idxB].arcHeight = 2.5f;
                bars[idxB].highlightTimer = 0.3f;
                bars[idxB].highlightColor = RED;
                bars[idxB].bounceScale = 1.25f;

                SpawnSwapParticles(bars[idxA].currentPos, bars[idxB].currentPos);
                PlayPitchSound(bars[idxA].value);
                break;
            }
            case STEP_SET: {
                swapCount++;
                int idx = step.indexA;
                int val = step.valueA;

                float normVal = (float)val / (float)elementCount;
                float h = normVal * maxHeight + 0.5f;

                bars[idx].value = val;
                bars[idx].color = GetHSVColor(normVal);
                bars[idx].highlightTimer = 0.25f;
                bars[idx].highlightColor = SKYBLUE;
                bars[idx].bounceScale = 1.15f;
                bars[idx].currentPos.y = h / 2.0f;
                bars[idx].targetPos.y = h / 2.0f;

                PlayPitchSound(val);
                break;
            }
            case STEP_PIVOT: {
                pivotIndex = step.indexA;
                if (pivotIndex >= 0) {
                    bars[pivotIndex].highlightTimer = 0.4f;
                    bars[pivotIndex].highlightColor = MAGENTA;
                }
                break;
            }
            case STEP_MARK_SORTED: {
                int idx = step.indexA;
                if (idx >= 0 && idx < (int)bars.size()) {
                    bars[idx].isSorted = true;
                    bars[idx].highlightTimer = 0.3f;
                    bars[idx].highlightColor = GREEN;
                    bars[idx].bounceScale = 1.2f;
                    SpawnSortedParticles(bars[idx].currentPos);
                    PlayPitchSound(bars[idx].value);
                }
                break;
            }
            default: break;
        }
    }

    void Update(float dt) {
        // Camera orbiting & controls
        if (autoRotateCamera) {
            cameraAngle += dt * 0.4f;
            float radius = 32.0f;
            camera.position.x = sinf(cameraAngle) * radius;
            camera.position.z = cosf(cameraAngle) * radius;
        } else {
            // Raylib default 3D camera controls with mouse right-click drag
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                Vector2 delta = GetMouseDelta();
                cameraAngle -= delta.x * 0.005f;
                float radius = Vector3Length((Vector3){ camera.position.x, 0.0f, camera.position.z });
                camera.position.x = sinf(cameraAngle) * radius;
                camera.position.z = cosf(cameraAngle) * radius;
                camera.position.y += delta.y * 0.1f;
                camera.position.y = Clamp(camera.position.y, 2.0f, 40.0f);
            }
            // Mouse wheel zoom
            float wheel = GetMouseWheelMove();
            if (wheel != 0.0f) {
                Vector3 dir = Vector3Subtract(camera.target, camera.position);
                float dist = Vector3Length(dir);
                if (dist > 5.0f || wheel < 0) {
                    camera.position = Vector3Add(camera.position, Vector3Scale(Vector3Normalize(dir), wheel * 2.0f));
                }
            }
        }
        UpdateCamera(&camera, CAMERA_CUSTOM);

        // Sorting Execution Timer
        if (isPlaying && !isFinished) {
            elapsedTime += dt;
            stepTimer += dt * speedMultiplier;
            while (stepTimer >= 1.0f && isPlaying) {
                stepTimer -= 1.0f;
                ExecuteNextStep();
            }
        }

        // Update 3D Bar Visual Animations (Interpolation, bounce, arc)
        for (auto& bar : bars) {
            // Smoothly lerp position towards targetPos
            float lerpSpeed = 15.0f * dt;
            bar.currentPos.x = Lerp(bar.currentPos.x, bar.targetPos.x, lerpSpeed);
            bar.currentPos.z = Lerp(bar.currentPos.z, bar.targetPos.z, lerpSpeed);

            // Arc jump calculation during swap
            if (bar.arcHeight > 0.01f) {
                bar.arcProgress += dt * 5.0f;
                if (bar.arcProgress >= 1.0f) {
                    bar.arcProgress = 1.0f;
                    bar.arcHeight = 0.0f;
                }
                // Parabola: y = 4 * height * p * (1 - p)
                float p = bar.arcProgress;
                float arcY = 4.0f * bar.arcHeight * p * (1.0f - p);
                bar.currentPos.z = sinf(p * PI) * 1.5f; // Slight Z-offset curve
            } else {
                bar.currentPos.z = Lerp(bar.currentPos.z, 0.0f, lerpSpeed);
            }

            // Timers & Bounce relaxation
            if (bar.highlightTimer > 0.0f) {
                bar.highlightTimer -= dt;
            }
            bar.bounceScale = Lerp(bar.bounceScale, 1.0f, 10.0f * dt);
        }

        // Update Particles
        for (int i = (int)particles.size() - 1; i >= 0; i--) {
            particles[i].life -= dt;
            if (particles[i].life <= 0.0f) {
                particles.erase(particles.begin() + i);
                continue;
            }
            particles[i].position = Vector3Add(particles[i].position, Vector3Scale(particles[i].velocity, dt));
            particles[i].velocity.y -= 9.8f * dt; // Gravity
        }
    }

    // ========================================================================
    // Drawing Engine
    // ========================================================================

    void Draw3D() {
        BeginMode3D(camera);

        // Draw futuristic grid pedestal
        DrawGrid(60, 1.5f);

        // Circular pedestal base platform
        DrawCylinder((Vector3){ 0.0f, -0.2f, 0.0f }, 25.0f, 25.0f, 0.4f, 32, (Color){ 25, 30, 45, 255 });
        DrawCylinderWires((Vector3){ 0.0f, -0.2f, 0.0f }, 25.0f, 25.0f, 0.4f, 32, (Color){ 50, 70, 110, 255 });

        // Draw 3D Bars
        for (int i = 0; i < (int)bars.size(); i++) {
            Bar& bar = bars[i];
            float normVal = (float)bar.value / (float)elementCount;
            float h = normVal * maxHeight + 0.5f;

            float w = barWidth * bar.bounceScale;
            float d = barWidth * bar.bounceScale;

            Vector3 renderPos = bar.currentPos;
            // Add arc jump to Y coordinate
            if (bar.arcHeight > 0.01f) {
                float p = bar.arcProgress;
                renderPos.y += 4.0f * bar.arcHeight * p * (1.0f - p);
            }

            // Base Bar Color vs Highlight Color
            Color barCol = bar.color;
            if (bar.isSorted) {
                barCol = ColorAlphaBlend(barCol, GREEN, (Color){ 0, 255, 100, 180 });
            }
            if (bar.highlightTimer > 0.0f) {
                float factor = bar.highlightTimer / 0.3f;
                barCol = ColorAlphaBlend(barCol, bar.highlightColor, (Color){ 255, 255, 255, (unsigned char)(200 * factor) });
            }

            // Render main 3D Bar Cuboid
            DrawCube(renderPos, w, h, d, barCol);
            DrawCubeWires(renderPos, w, h, d, (Color){ 10, 15, 25, 180 });

            // Glowing top cap
            Vector3 capPos = renderPos;
            capPos.y += h / 2.0f + 0.05f;
            DrawCube(capPos, w * 1.02f, 0.1f, d * 1.02f, WHITE);

            // Pivot indicator floating beacon
            if (i == pivotIndex) {
                Vector3 beaconPos = renderPos;
                beaconPos.y += h / 2.0f + 1.2f + sinf(GetTime() * 8.0f) * 0.3f;
                DrawCylinder(beaconPos, 0.0f, 0.6f, 1.0f, 8, MAGENTA);
                DrawSphere(beaconPos, 0.3f, PINK);
            }
        }

        // Draw 3D Particles
        for (const auto& p : particles) {
            float alpha = p.life / p.maxLife;
            Color pCol = ColorAlpha(p.color, alpha);
            DrawSphere(p.position, p.size * alpha, pCol);
        }

        EndMode3D();
    }

    void Draw2DUI() {
        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();

        // Header Panel
        DrawRectangle(0, 0, screenW, 60, (Color){ 15, 20, 30, 230 });
        DrawRectangleLines(0, 0, screenW, 60, (Color){ 40, 50, 80, 255 });

        DrawText("3D SORTING ALGORITHM VISUALIZER", 20, 16, 24, RAYWHITE);
        DrawText("Raylib C++ Edition", 440, 22, 14, SKYBLUE);

        // Left Controls Panel
        int panelW = 280;
        DrawRectangle(15, 75, panelW, 460, (Color){ 20, 25, 40, 220 });
        DrawRectangleLines(15, 75, panelW, 460, (Color){ 50, 75, 120, 255 });

        int y = 90;
        DrawText("ALGORITHM SELECTOR", 30, y, 14, GOLD);
        y += 25;

        for (int i = 0; i < ALG_COUNT; i++) {
            bool isSelected = (currentAlg == (AlgorithmType)i);
            Color btnCol = isSelected ? (Color){ 0, 120, 215, 255 } : (Color){ 40, 48, 65, 255 };
            Rectangle btnRect = { 30.0f, (float)y, 250.0f, 26.0f };

            if (CheckCollisionPointRec(GetMousePosition(), btnRect)) {
                btnCol = isSelected ? (Color){ 30, 144, 255, 255 } : (Color){ 60, 72, 95, 255 };
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    currentAlg = (AlgorithmType)i;
                    ResetArray();
                }
            }
            DrawRectangleRec(btnRect, btnCol);
            DrawRectangleLinesEx(btnRect, 1, isSelected ? WHITE : DARKGRAY);
            DrawText(ALG_NAMES[i], 40, y + 5, 14, isSelected ? WHITE : LIGHTGRAY);
            y += 30;
        }

        y += 10;
        DrawText("ARRAY DISTRIBUTION", 30, y, 14, GOLD);
        y += 25;

        for (int i = 0; i < DIST_COUNT; i++) {
            bool isSelected = (currentDist == (DistributionType)i);
            Color btnCol = isSelected ? (Color){ 0, 150, 130, 255 } : (Color){ 40, 48, 65, 255 };
            Rectangle btnRect = { 30.0f, (float)y, 120.0f, 24.0f };
            if (i % 2 == 1) btnRect.x = 160.0f;
            else if (i > 0) y += 28;

            if (CheckCollisionPointRec(GetMousePosition(), btnRect)) {
                btnCol = isSelected ? (Color){ 20, 180, 150, 255 } : (Color){ 60, 72, 95, 255 };
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    currentDist = (DistributionType)i;
                    ResetArray();
                }
            }
            DrawRectangleRec(btnRect, btnCol);
            DrawRectangleLinesEx(btnRect, 1, isSelected ? WHITE : DARKGRAY);
            DrawText(DIST_NAMES[i], (int)btnRect.x + 8, (int)btnRect.y + 4, 12, isSelected ? WHITE : LIGHTGRAY);
        }
        y += 35;

        // Playback Buttons (Play/Pause, Step, Reset)
        DrawText("CONTROLS", 30, y, 14, GOLD);
        y += 25;

        // Play/Pause Button
        Rectangle playRect = { 30.0f, (float)y, 75.0f, 32.0f };
        Color playCol = isPlaying ? ORANGE : GREEN;
        if (CheckCollisionPointRec(GetMousePosition(), playRect)) {
            playCol = isPlaying ? RED : LIME;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (isFinished) ResetArray();
                isPlaying = !isPlaying;
            }
        }
        DrawRectangleRec(playRect, playCol);
        DrawRectangleLinesEx(playRect, 1, WHITE);
        DrawText(isPlaying ? "PAUSE" : "PLAY", (int)playRect.x + 14, (int)playRect.y + 8, 14, BLACK);

        // Step Button
        Rectangle stepRect = { 115.0f, (float)y, 75.0f, 32.0f };
        Color stepCol = (Color){ 70, 90, 140, 255 };
        if (CheckCollisionPointRec(GetMousePosition(), stepRect)) {
            stepCol = (Color){ 90, 120, 180, 255 };
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                isPlaying = false;
                ExecuteNextStep();
            }
        }
        DrawRectangleRec(stepRect, stepCol);
        DrawRectangleLinesEx(stepRect, 1, WHITE);
        DrawText("STEP", (int)stepRect.x + 20, (int)stepRect.y + 8, 14, WHITE);

        // Reset / Shuffle Button
        Rectangle resetRect = { 200.0f, (float)y, 80.0f, 32.0f };
        Color resetCol = (Color){ 160, 50, 50, 255 };
        if (CheckCollisionPointRec(GetMousePosition(), resetRect)) {
            resetCol = (Color){ 200, 70, 70, 255 };
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                ResetArray();
            }
        }
        DrawRectangleRec(resetRect, resetCol);
        DrawRectangleLinesEx(resetRect, 1, WHITE);
        DrawText("RESET", (int)resetRect.x + 16, (int)resetRect.y + 8, 14, WHITE);

        y += 45;

        // Sliders (Speed & Size)
        DrawText(TextFormat("Speed: %.0f steps/sec", speedMultiplier), 30, y, 12, LIGHTGRAY);
        y += 18;
        Rectangle speedSlider = { 30.0f, (float)y, 250.0f, 12.0f };
        DrawRectangleRec(speedSlider, DARKGRAY);
        float speedNorm = (speedMultiplier - 1.0f) / 99.0f;
        DrawRectangle(30, y, (int)(250 * speedNorm), 12, SKYBLUE);
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), (Rectangle){ 30, (float)y - 5, 250, 22 })) {
            float clickNorm = (GetMousePosition().x - 30.0f) / 250.0f;
            speedMultiplier = Clamp(1.0f + clickNorm * 99.0f, 1.0f, 100.0f);
        }
        y += 24;

        DrawText(TextFormat("Array Size: %d elements", elementCount), 30, y, 12, LIGHTGRAY);
        y += 18;
        Rectangle sizeSlider = { 30.0f, (float)y, 250.0f, 12.0f };
        DrawRectangleRec(sizeSlider, DARKGRAY);
        float sizeNorm = (float)(elementCount - 10) / 50.0f;
        DrawRectangle(30, y, (int)(250 * sizeNorm), 12, GREEN);
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), (Rectangle){ 30, (float)y - 5, 250, 22 })) {
            float clickNorm = (GetMousePosition().x - 30.0f) / 250.0f;
            int newSize = Clamp(10 + (int)(clickNorm * 50.0f), 10, 60);
            if (newSize != elementCount) {
                elementCount = newSize;
                ResetArray();
            }
        }

        // Top Right Stats Panel
        int statsW = 240;
        DrawRectangle(screenW - statsW - 15, 75, statsW, 165, (Color){ 20, 25, 40, 220 });
        DrawRectangleLines(screenW - statsW - 15, 75, statsW, 165, (Color){ 50, 75, 120, 255 });

        int sy = 90;
        DrawText("STATISTICS", screenW - statsW, sy, 14, GOLD);
        sy += 24;
        DrawText(TextFormat("Status: %s", isFinished ? "COMPLETED!" : (isPlaying ? "Sorting..." : "Paused")), screenW - statsW, sy, 13, isFinished ? GREEN : (isPlaying ? YELLOW : RED));
        sy += 20;
        DrawText(TextFormat("Comparisons: %d", comparisonCount), screenW - statsW, sy, 13, LIGHTGRAY);
        sy += 20;
        DrawText(TextFormat("Swaps / Writes: %d", swapCount), screenW - statsW, sy, 13, LIGHTGRAY);
        sy += 20;
        DrawText(TextFormat("Step Progress: %zu / %zu", currentStepIndex, steps.size()), screenW - statsW, sy, 13, LIGHTGRAY);
        sy += 20;
        DrawText(TextFormat("Elapsed Time: %.2f s", elapsedTime), screenW - statsW, sy, 13, LIGHTGRAY);

        // Bottom Right Legend & Controls Help
        int legendW = 260;
        DrawRectangle(screenW - legendW - 15, screenH - 120, legendW, 105, (Color){ 20, 25, 40, 220 });
        DrawRectangleLines(screenW - legendW - 15, screenH - 120, legendW, 105, (Color){ 50, 75, 120, 255 });

        int ly = screenH - 110;
        DrawText("CAMERA & AUDIO KEYS", screenW - legendW, ly, 12, GOLD);
        ly += 18;
        DrawText("Right Drag: Orbit 3D Camera", screenW - legendW, ly, 11, LIGHTGRAY);
        ly += 16;
        DrawText("Mouse Wheel: Zoom Camera", screenW - legendW, ly, 11, LIGHTGRAY);
        ly += 16;
        DrawText("Space: Play/Pause | R: Reset | S: Step", screenW - legendW, ly, 11, LIGHTGRAY);
        ly += 16;
        DrawText(TextFormat("C: Auto-Rotate Camera [%s]", autoRotateCamera ? "ON" : "OFF"), screenW - legendW, ly, 11, autoRotateCamera ? GREEN : LIGHTGRAY);

        // Sound Toggle Button (Bottom Left)
        Rectangle soundBtn = { 15.0f, (float)(screenH - 45), 140.0f, 30.0f };
        Color soundCol = soundEnabled ? (Color){ 40, 120, 60, 255 } : (Color){ 120, 40, 40, 255 };
        if (CheckCollisionPointRec(GetMousePosition(), soundBtn)) {
            soundCol = soundEnabled ? (Color){ 60, 160, 80, 255 } : (Color){ 160, 60, 60, 255 };
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                soundEnabled = !soundEnabled;
            }
        }
        DrawRectangleRec(soundBtn, soundCol);
        DrawRectangleLinesEx(soundBtn, 1, WHITE);
        DrawText(TextFormat("Sound: %s", soundEnabled ? "ON [M]" : "OFF [M]"), (int)soundBtn.x + 12, (int)soundBtn.y + 7, 13, WHITE);
    }

    void HandleKeyboardShortcuts() {
        if (IsKeyPressed(KEY_SPACE)) {
            if (isFinished) ResetArray();
            isPlaying = !isPlaying;
        }
        if (IsKeyPressed(KEY_R)) {
            ResetArray();
        }
        if (IsKeyPressed(KEY_S)) {
            isPlaying = false;
            ExecuteNextStep();
        }
        if (IsKeyPressed(KEY_C)) {
            autoRotateCamera = !autoRotateCamera;
        }
        if (IsKeyPressed(KEY_M)) {
            soundEnabled = !soundEnabled;
        }
    }
};

// ============================================================================
// Main Function
// ============================================================================

int main() {
    // Enable Anti-Aliasing (MSAA 4x)
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "Playful 3D Sorting Algorithm Visualizer (Raylib C++)");
    SetTargetFPS(60);

    Visualizer vis;
    vis.InitAudio();
    vis.ResetArray();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        vis.HandleKeyboardShortcuts();
        vis.Update(dt);

        BeginDrawing();
        ClearBackground((Color){ 12, 16, 25, 255 }); // Dark sleek background

        vis.Draw3D();
        vis.Draw2DUI();

        EndDrawing();
    }

    vis.CleanAudio();
    CloseWindow();

    return 0;
}
