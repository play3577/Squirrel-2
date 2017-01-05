#makefileを作って　g++でのビルドに挑戦！！
#全然わからん(＾◇＾)

#コンパイラの指定
COMPILER = g++
#コンパイル時のフラグ
CFLAGS　= -std=c++14 -Wextra -Ofast -fno-exceptions -fno-rtti 
LDFLAGS  = -lpthread
LIBS     =
INCLUDE  =  #-I../include
#実行ファイル名
TARGET= squirrel
ifeq ($(OS),Windows_NT)
  LDFLAGS += -static
endif
#objectfileの作成位置
OBJDIR   = ../obj
#SOURCEとHEADERとOBJECTS
SOURCES := $(wildcard *.cpp)
HEADER := $(wildcard *.h)
OBJECTS  = $(addprefix $(OBJDIR)/, $(SOURCES:.cpp=.o))
DEPENDS  = $(OBJECTS:.o=.d)

#targetが引数で与えられた（または引数があたえられなかった）ときに何をするか
$(TARGET): $(OBJECTS) $(LIBS)
	$(COMPILER) -o $@ $^ $(LDFLAGS) $(CFLAGS)

#オブジェクトファイルの作成
$(OBJDIR)/%.o: %.cpp
	@[ -d $(OBJDIR) ] ||  mkdir -p $(OBJDIR)
	$(COMPILER) $(CFLAGS) $(INCLUDE) -o $@ -c $<



all: clean $(TARGET)

release:
	$(MAKE) CFLAGS='$(CFLAGS)' LDFLAGS='$(LDFLAGS) -flto' $(TARGET)


test:
	echo $(OBJECTS)

-include $(DEPENDS)
