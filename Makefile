CXX = g++
CXXFLAGS = -std=c++17 -O2
LIBS = -lportaudio -lm

TARGET = synth
SRCS = main.cpp poly_synth.cpp voice.cpp harmonic_osc.cpp vcf.cpp effects/reverb_effect.cpp
OBJS = $(SRCS:.cpp=.o)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -I. -o $(TARGET) $(OBJS) $(LIBS)

clean:
	rm -f $(TARGET) $(OBJS)