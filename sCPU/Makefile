TOPNAME = top
NXDC_FILES = constr/top.nxdc
INC_PATH ?=

VERILATOR = verilator
VERILATOR_CFLAGS += -MMD --build --trace-fst -cc \
				-O3 --x-assign fast --x-initial fast --noassert

BUILD_DIR = ./build
OBJ_DIR = $(BUILD_DIR)/obj_dir
BIN ?= /dev/null

default: $(BUILD_DIR)/$(TOPNAME)

$(shell mkdir -p $(BUILD_DIR))

SRC_AUTO_BIND = $(abspath $(BUILD_DIR)/auto_bind.cpp)
$(SRC_AUTO_BIND): $(NXDC_FILES)
	python3 $(NVBOARD_HOME)/scripts/auto_pin_bind.py $^ $@

VSRCS = $(shell find $(abspath ./vsrc) -name "*.v")
CSRCS = $(shell find $(abspath ./csrc) -name "*.c" -or -name "*.cc" -or -name "*.cpp")
CSRCS += $(SRC_AUTO_BIND)

include $(NVBOARD_HOME)/scripts/nvboard.mk

INCFLAGS = $(addprefix -I, $(INC_PATH))
CXXFLAGS += $(INCFLAGS) -DTOP_NAME="\"V$(TOPNAME)\""

$(BUILD_DIR)/$(TOPNAME): $(VSRCS) $(CSRCS) $(NVBOARD_ARCHIVE)
	@rm -rf $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_CFLAGS) \
		--top-module $(TOPNAME) $^ \
		$(addprefix -CFLAGS , $(CXXFLAGS)) $(addprefix -LDFLAGS , $(LDFLAGS)) \
		--Mdir $(OBJ_DIR) --exe -o $(abspath $@)

all: default

# ------------------------
# 运行模式
# ------------------------
run: $(BUILD_DIR)/$(TOPNAME)
	@if [ ! -f "$(BIN)" ]; then \
	    echo "❌ 镜像文件不存在: $(BIN)"; \
	    exit 1; \
	fi
	@echo "🚀 [No GUI] 启动 CPU 仿真"
	./build/$(TOPNAME) $(BIN) +fst

run-gui: $(BUILD_DIR)/$(TOPNAME)
	@if [ ! -f "$(BIN)" ]; then \
	    echo "❌ 镜像文件不存在: $(BIN)"; \
	    exit 1; \
	fi
	@echo "🎮 [GUI] 启动 CPU 仿真 (NVBoard)"
	./build/$(TOPNAME) $(BIN) +fst +gui

clean:
	rm -rf $(BUILD_DIR)

.PHONY: default all clean run run-gui
