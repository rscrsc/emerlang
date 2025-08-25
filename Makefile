CFLAGS += -std=c++20 -O3 -Wall -Wextra -Wpedantic -I./ -I./imgui
CFLAGS += -MMD -MP
LDFLAGS += -L./imgui -limgui -pthread -sUSE_PTHREADS=1 -sPTHREAD_POOL_SIZE=3 \
		   -sPROXY_TO_PTHREAD=1 -sOFFSCREENCANVAS_SUPPORT=1

IMGUI_SRCS := $(wildcard imgui/*.cpp) imgui/backends/imgui_impl_opengl3.cpp
IMGUI_OBJS := $(patsubst %.cpp,%.o,$(filter %.cpp,$(IMGUI_SRCS)))
IMGUI_AR := imgui/libimgui.a
SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,%.o,$(filter %.cpp,$(SRCS)))
MDFS := $(patsubst %.cpp,%.d,$(filter %.cpp,$(SRCS)))

WEB_OUTPUT_DIR := public
WEB_OUTPUT_FILES := emerlang.js emerlang.wasm
WEB_OUTPUT := $(addprefix $(WEB_OUTPUT_DIR)/,$(WEB_OUTPUT_FILES))

.PHONY: prune clean
.DEFAULT_GOAL := all

all: $(WEB_OUTPUT)
	python serve.py

# $(OBJS): dep

define GEN_template
$(WEB_OUTPUT): $(OBJS) $(IMGUI_AR)
	@mkdir -p $(WEB_OUTPUT_DIR)
	@em++ $$(filter %.o,$$^) \
		$(LDFLAGS) \
		-o $(filter %.js,$(WEB_OUTPUT)) \
		-sUSE_WEBGL2=1 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 \
		-sENVIRONMENT=web \
		-sALLOW_MEMORY_GROWTH=1 \
		-sNO_EXIT_RUNTIME=1 \
		-sASSERTIONS=0 \
		-sEXPORTED_FUNCTIONS='["_main"]' \
		-sEXPORTED_RUNTIME_METHODS='["ccall","cwrap"]'
endef
$(eval $(GEN_template))

$(OBJS): %.o: %.cpp
	@em++ $(CFLAGS) -c $< -o $@

$(IMGUI_AR): $(IMGUI_OBJS)
	@emar rcs $@ $^

$(IMGUI_OBJS): %.o: %.cpp
	@em++ $(CFLAGS) -c $< -o $@

prune:
	@rm -f $(OBJS) $(MDFS)

clean: prune
	@rm -f $(WEB_OUTPUT)

-include $(MDFS)
# include dep.mk

