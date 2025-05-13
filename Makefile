CXX = g++
# CXXFLAGS の -I. を削除または変更
# プロジェクトのルートディレクトリからの相対パスで指定
NLOHMANN_JSON_INCLUDE_DIR = ./include # json.hpp を ./include/nlohmann/json.hpp に置いた場合
RTMIDI_INCLUDE_DIR = /usr/local/include/rtmidi # RtMidi をシステムにインストールした場合の例
MIDIFILE_INCLUDE_DIR = ./Midifile/include # Midifile をプロジェクト内に置いた場合の例

CXXFLAGS = -std=c++17 -O2 -I$(NLOHMANN_JSON_INCLUDE_DIR) -I$(RTMIDI_INCLUDE_DIR) -I$(MIDIFILE_INCLUDE_DIR) -I. # -I. はカレント(synth)ディレクトリ用
LIBS = -lportaudio -lm -lrtmidi # Midifile はソースからコンパイルする場合は不要

TARGET = synth
# Midifile のソースもここに追加するか、ライブラリとしてリンク
# Midifile/src/*.cpp をコンパイル対象に加える例
MIDIFILE_SRCS = $(wildcard Midifile/src/*.cpp) # Midifileのソースファイル群
SRCS = main.cpp poly_synth.cpp voice.cpp harmonic_osc.cpp vcf.cpp effects/reverb_effect.cpp \
       $(MIDIFILE_SRCS)

OBJS = $(SRCS:.cpp=.o)

# $(TARGET) のリンク行も確認
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

clean:
	rm -f $(TARGET) $(OBJS)