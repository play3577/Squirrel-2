#makefileを作って　g++でのビルドに挑戦！！
#全然わからん(＾◇＾)

#コンパイラの指定
COMPILER = g++
#コンパイル時のフラグ
CFLAGS　= -std=c++14 -Wextra -Ofast 
#実行ファイル名
TARGET= squirrel
#objectfileの作成位置
OBJDIR= ../obj 
#SOURCEとHEADERとOBJECTS
SOURCES := $(wildcard *.cpp)
HEADER := $(wildcard *.h)
OBJECTS  = $(addprefix $(OBJDIR)/, $(SOURCES:.cpp=.o))
DEPENDS  = $(OBJECTS:.o=.d)

#targetが引数で与えられた（または引数があたえられなかった）ときに何をするか
$(TARGET): $(OBJECTS)
	$(COMPILER) -o $@ $^ $(CFLAGS)


#オブジェクトファイルの作成
$(OBJDIR)/%.o: %.cpp
	#@[ -d $(OBJDIR) ] || mkdir -p $(OBJDIR)
	$(COMPILER) $(CFLAGS)  -o $@ -c $<

test:
	echo $(TARGET)

debug:
	$(MAKE) CFLAGS='$(CFLAGS) -D_DEBUG -flto' $(TARGET)

release:
	$(MAKE) CFLAGS='$(CFLAGS) -flto' $(TARGET)

