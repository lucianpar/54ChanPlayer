/*
54-Channel Audio Playback System
Plays back a multichannel audio file with all channels mapped to individual outputs.
Includes GUI controls for playback, pause, loop, and rewind.
Includes real-time dB meters for all 54 channels.
*/

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>
#include "al/app/al_App.hpp"
#include "al/io/al_File.hpp"
#include "al/io/al_Imgui.hpp"
#include "al/sound/al_SoundFile.hpp"

using namespace al;

struct MyApp : App {
  SoundFile soundFile;
  uint64_t frameCounter = 0;
  std::vector<float> buffer;
  
  // Playback controls
  bool playing = false;
  bool loop = true;
  float gain = 0.5f;
  
  // Audio file info
  int numChannels = 54;
  int expectedChannels = 54;
  std::string audioFolder = "/../sourceAudio/";
  std::string audioFileName = "Porous-sonoPleth-Render.wav";
  
  // Metering
  std::vector<float> channelLevels;  // Linear amplitude for each channel
  std::vector<float> channelPeaks;   // Peak hold for each channel
  int peakHoldFrames = 24;           // How long to hold peaks (in render frames)
  std::vector<int> peakHoldCounters; // Counter for peak hold
  float meterDecayRate = 0.95f;      // How fast meters decay
  bool showMeters = true;
  
  void onInit() override {
    // Build audio file path using currentPath (like ShaderPlayback)
    std::string audioPath = al::File::currentPath() + audioFolder + audioFileName;
    
    std::cout << "\n=== 54-Channel Audio Player ===" << std::endl;
    std::cout << "Current path: " << al::File::currentPath() << std::endl;
    std::cout << "Loading audio file: " << audioPath << std::endl;
    
    if (!soundFile.open(audioPath.c_str())) {
      std::cerr << "✗ ERROR: Could not open file: " << audioPath << std::endl;
      std::cerr << "Please update the audioFolder or audioFileName variables." << std::endl;
      quit();
      return;
    }
    
    std::cout << "✓ Audio file loaded successfully" << std::endl;
    std::cout << "  Sample rate: " << soundFile.sampleRate << " Hz" << std::endl;
    std::cout << "  Channels: " << soundFile.channels << std::endl;
    std::cout << "  Frame count: " << soundFile.frameCount << std::endl;
    std::cout << "  Duration: " << (double)soundFile.frameCount / soundFile.sampleRate << " seconds" << std::endl;
    
    numChannels = soundFile.channels;
    
    if (numChannels != expectedChannels) {
      std::cerr << "⚠ WARNING: Expected " << expectedChannels << " channels but file has " 
                << numChannels << " channels." << std::endl;
      std::cout << "Will use " << numChannels << " output channels." << std::endl;
    }
    
    // Allocate buffer for deinterleaving
    int framesPerBuffer = 512; // Will be updated in onSound
    buffer.resize(framesPerBuffer * numChannels);
    
    // Initialize metering
    channelLevels.resize(numChannels, 0.0f);
    channelPeaks.resize(numChannels, 0.0f);
    peakHoldCounters.resize(numChannels, 0);
    
    frameCounter = 0;
  }
  
  void onCreate() override { 
    imguiInit(); 
  }
  
  void onDraw(Graphics& g) override {
    imguiBeginFrame();
    
    ImGui::Begin("54-Channel Audio Player");
    ImGui::Text("File Info:");
    ImGui::Text("  Channels: %d", numChannels);
    ImGui::Text("  Sample Rate: %d Hz", soundFile.sampleRate);
    ImGui::Text("  Duration: %.2f seconds", (double)soundFile.frameCount / soundFile.sampleRate);
    
    ImGui::Separator();
    ImGui::Text("Playback:");
    ImGui::Text("  Current Frame: %llu / %llu", frameCounter, soundFile.frameCount);
    ImGui::Text("  Current Time: %.2f / %.2f seconds", 
                (double)frameCounter / soundFile.sampleRate,
                (double)soundFile.frameCount / soundFile.sampleRate);
    
    ImGui::Separator();
    ImGui::Text("Controls:");
    
    if (ImGui::Button(playing ? "⏸ Pause" : "▶ Play")) {
      playing = !playing;
    }
    
    ImGui::SameLine();
    if (ImGui::Button("⏹ Stop")) {
      playing = false;
      frameCounter = 0;
    }
    
    ImGui::SameLine();
    if (ImGui::Button("⏮ Rewind")) {
      frameCounter = 0;
    }
    
    if (ImGui::Checkbox("Loop", &loop)) {
      std::cout << "Loop: " << (loop ? "ON" : "OFF") << std::endl;
    }
    
    if (ImGui::SliderFloat("Gain", &gain, 0.0f, 1.0f)) {
      std::cout << "Gain: " << gain << std::endl;
    }
    
    ImGui::Separator();
    ImGui::Checkbox("Show Channel Meters", &showMeters);
    
    if (showMeters) {
      ImGui::Text("Channel Levels (dB):");
      
      // Display meters in a scrollable area
      ImGui::BeginChild("Meters", ImVec2(0, 400), true);
      
      for (int ch = 0; ch < numChannels; ch++) {
        // Convert linear amplitude to dB
        float levelDB = -120.0f; // Silence floor
        if (channelLevels[ch] > 0.0f) {
          levelDB = 20.0f * log10f(channelLevels[ch]);
        }
        
        float peakDB = -120.0f;
        if (channelPeaks[ch] > 0.0f) {
          peakDB = 20.0f * log10f(channelPeaks[ch]);
        }
        
        // Clamp to reasonable display range
        levelDB = (levelDB < -60.0f) ? -60.0f : levelDB;
        peakDB = (peakDB < -60.0f) ? -60.0f : peakDB;
        
        // Normalize to 0-1 range for display (-60dB to 0dB)
        float levelNorm = (levelDB + 60.0f) / 60.0f;
        float peakNorm = (peakDB + 60.0f) / 60.0f;
        
        // Color based on level (green -> yellow -> red)
        ImVec4 color;
        if (levelNorm < 0.5f) {
          color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
        } else if (levelNorm < 0.85f) {
          color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
        } else {
          color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
        }
        
        // Channel label and dB value
        ImGui::Text("Ch %2d:", ch + 1);
        ImGui::SameLine(60);
        
        // Progress bar for meter
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
        ImGui::ProgressBar(levelNorm, ImVec2(200, 0), "");
        ImGui::PopStyleColor();
        
        ImGui::SameLine();
        
        // Peak indicator (small vertical line)
        if (peakNorm > 0.01f) {
          ImGui::Text("|");
        } else {
          ImGui::Text(" ");
        }
        
        ImGui::SameLine();
        
        // Show dB value
        if (levelDB > -60.0f) {
          ImGui::Text("%5.1f dB", levelDB);
        } else {
          ImGui::Text("  -inf");
        }
      }
      
      ImGui::EndChild();
    }
    
    ImGui::End();
    
    imguiEndFrame();
    g.clear(0, 0, 0);
    imguiDraw();
  }
  
  void onSound(AudioIOData& io) override {
    if (!soundFile.data.size()) {
      // No file loaded, output silence
      while (io()) {
        for (int ch = 0; ch < io.channelsOut(); ch++) {
          io.out(ch) = 0.0f;
        }
      }
      return;
    }
    
    uint64_t numFrames = io.framesPerBuffer();
    
    // Resize buffer if needed
    if (buffer.size() < numFrames * numChannels) {
      buffer.resize(numFrames * numChannels);
    }
    
    // If not playing, output silence
    if (!playing) {
      while (io()) {
        for (int ch = 0; ch < io.channelsOut(); ch++) {
          io.out(ch) = 0.0f;
        }
      }
      return;
    }
    
    // Check if we're at the end
    if (frameCounter >= soundFile.frameCount) {
      if (loop) {
        frameCounter = 0;
      } else {
        playing = false;
        while (io()) {
          for (int ch = 0; ch < io.channelsOut(); ch++) {
            io.out(ch) = 0.0f;
          }
        }
        return;
      }
    }
    
    // Adjust numFrames if we're near the end
    if (frameCounter + numFrames > soundFile.frameCount) {
      numFrames = soundFile.frameCount - frameCounter;
    }
    
    // Get pointer to current frame in the soundfile
    float* frames = soundFile.getFrame(frameCounter);
    
    // Copy interleaved data to buffer
    for (uint64_t i = 0; i < numFrames * numChannels; i++) {
      buffer[i] = frames[i];
    }
    
    // Deinterleave and output to all channels
    int outputChannels = (numChannels < io.channelsOut()) ? numChannels : io.channelsOut();
    
    // Reset channel levels for this buffer
    std::vector<float> maxLevels(numChannels, 0.0f);
    
    for (uint64_t frame = 0; frame < numFrames; frame++) {
      for (int ch = 0; ch < outputChannels; ch++) {
        float sample = buffer[frame * numChannels + ch] * gain;
        io.out(ch, frame) = sample;
        
        // Track max level for metering
        float absSample = fabsf(sample);
        if (absSample > maxLevels[ch]) {
          maxLevels[ch] = absSample;
        }
      }
      
      // If we have fewer file channels than outputs, fill remaining with silence
      for (int ch = outputChannels; ch < io.channelsOut(); ch++) {
        io.out(ch, frame) = 0.0f;
      }
    }
    
    // Update meters with max levels from this buffer
    for (int ch = 0; ch < outputChannels; ch++) {
      // Smooth decay for current level
      channelLevels[ch] = channelLevels[ch] * meterDecayRate;
      
      // Update with new max if higher
      if (maxLevels[ch] > channelLevels[ch]) {
        channelLevels[ch] = maxLevels[ch];
      }
      
      // Update peak hold
      if (maxLevels[ch] > channelPeaks[ch]) {
        channelPeaks[ch] = maxLevels[ch];
        peakHoldCounters[ch] = peakHoldFrames;
      } else {
        // Decay peak hold
        if (peakHoldCounters[ch] > 0) {
          peakHoldCounters[ch]--;
        } else {
          channelPeaks[ch] = channelPeaks[ch] * meterDecayRate;
        }
      }
    }
    
    // Fill remaining frames with silence if we read fewer frames
    for (uint64_t frame = numFrames; frame < io.framesPerBuffer(); frame++) {
      for (int ch = 0; ch < io.channelsOut(); ch++) {
        io.out(ch, frame) = 0.0f;
      }
    }
    
    frameCounter += numFrames;
  }
  
  bool onKeyDown(const Keyboard& k) override {
    if (k.key() == ' ') {
      playing = !playing;
      std::cout << (playing ? "▶ Playing" : "⏸ Paused") << std::endl;
      return true;
    }
    if (k.key() == 'r' || k.key() == 'R') {
      frameCounter = 0;
      std::cout << "⏮ Rewound to beginning" << std::endl;
      return true;
    }
    if (k.key() == 'l' || k.key() == 'L') {
      loop = !loop;
      std::cout << "Loop: " << (loop ? "ON" : "OFF") << std::endl;
      return true;
    }
    return false;
  }
  
  void onExit() override { 
    imguiShutdown(); 
  }
};

int main() {
  MyApp app;
  
  // Configure audio for 54 output channels, 0 input channels
  // Adjust sample rate and buffer size as needed
  app.configureAudio(48000,  // Sample rate (adjust to match your file)
                     512,    // Buffer size
                     54,     // Output channels
                     0);     // Input channels
  
  std::cout << "\n=== Audio Configuration ===" << std::endl;
  std::cout << "Output channels: 54" << std::endl;
  std::cout << "Sample rate: 48000 Hz" << std::endl;
  std::cout << "Buffer size: 512 frames" << std::endl;
  std::cout << "\nKeyboard shortcuts:" << std::endl;
  std::cout << "  SPACE - Play/Pause" << std::endl;
  std::cout << "  R - Rewind" << std::endl;
  std::cout << "  L - Toggle Loop" << std::endl;
  std::cout << "\n" << std::endl;
  
  app.start();
  return 0;
}
