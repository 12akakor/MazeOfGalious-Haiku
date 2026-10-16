# Native build for Haiku / RenkuOS (x86_64). Run on Haiku/RenkuOS itself:
#
#   make            builds ./MazeOfGalious (Be API only)
#   make run        builds and starts the game (windowed)
#   make test       builds a test binary and compares 1400 frames with the
#                   golden reference (tests/golden), plus unit tests
#   make package    builds mazeofgalious-0.63.3-3-x86_64.hpkg (and the extras package)

NAME     = MazeOfGalious
CXX     ?= g++
CC      ?= gcc
CXXFLAGS = -O2 -g -std=gnu++17 -Isrc -Wall -Wno-write-strings -Wno-unused \
	-Wno-sign-compare -Wno-parentheses -Wno-format-security \
	-Wno-misleading-indentation -Wno-char-subscripts -Wno-unused-result \
	-Wno-comment -Wno-format-overflow -Wno-format-truncation \
	-Wno-stringop-truncation
CFLAGS   = -O2 -g -w
LIBS     = -lbe -lmedia

GAME_SRCS = $(wildcard src/*.cpp)
PLAT_SRCS = src/platform/audio_mixer.cpp src/platform/platform_common.cpp \
	src/platform/video_scaler.cpp src/platform/keynames.cpp src/platform/resampler.cpp \
	src/platform/platform_haiku.cpp

OBJS      = $(patsubst src/%.cpp,obj/haiku/%.o,$(GAME_SRCS) $(PLAT_SRCS)) \
	obj/haiku/third_party/stb_vorbis.o
TEST_OBJS = $(patsubst src/%.cpp,obj/haiku-test/%.o,$(GAME_SRCS) $(PLAT_SRCS)) \
	obj/haiku-test/third_party/stb_vorbis.o obj/haiku-test/test/testhooks.o

RDEF = build/haiku/MazeOfGalious.rdef
RSRC = obj/haiku/$(NAME).rsrc

all: $(NAME)

$(NAME): $(OBJS) $(RSRC)
	$(CXX) $(OBJS) -o $@ $(LIBS)
	xres -o $@ $(RSRC)
	mimeset -f $@

$(NAME)-test: $(TEST_OBJS) $(RSRC)
	$(CXX) $(TEST_OBJS) -o $@ $(LIBS)
	xres -o $@ $(RSRC)
	mimeset -f $@

$(RSRC): $(RDEF) build/haiku/icon.rdef
	@mkdir -p $(dir $@)
	cat $(RDEF) build/haiku/icon.rdef > obj/haiku/$(NAME).rdef
	rc -o $@ obj/haiku/$(NAME).rdef

obj/haiku/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

obj/haiku-test/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -DMOG_TESTHOOKS -include src/test/testhooks.h -c $< -o $@

obj/%/third_party/stb_vorbis.o: src/third_party/stb_vorbis.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(NAME)
	./$(NAME) --window

UNIT_SRCS = tests/unit/unit_tests.cpp src/filehandling.cpp \
	src/platform/audio_mixer.cpp src/platform/platform_common.cpp \
	src/platform/video_scaler.cpp src/platform/keynames.cpp src/platform/resampler.cpp \
	src/loadpcx.cpp

obj/unit_tests: $(UNIT_SRCS) obj/haiku/third_party/stb_vorbis.o
	@mkdir -p obj
	$(CXX) $(CXXFLAGS) $^ -o $@

test: $(NAME)-test obj/unit_tests
	./obj/unit_tests data
	sh tools/check-golden.sh ./$(NAME)-test basic

package: $(NAME)
	sh tools/make-package.sh

clean:
	rm -rf obj $(NAME) $(NAME)-test *.hpkg

.PHONY: all run test package clean
