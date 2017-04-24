#http://boysenberrypi.hatenadiary.jp/entry/2014/03/15/113703

#-----------------------------------最初に後で使う変数の定義を行っておく.-------------------------------------
COMPILER = g++
#コンパイラ　オプション（コンパイラ　フラグ）
#-MMD　デバッグ情報を作成
#-MP  ソースファイルがインクルードするヘッダファイルが依存するものがないとして依存関係を出力する。
#-MMDと-MPのオプションを使用しているのは，ヘッダファイルが更新された時にmakeが行われるようにするためである。
#-Ofast 最も最適化するためのオプション？
#例外を完全に除去-fno-exceptions
#Wextra 厳しめの警告を出す
CFLAGS	= -std=c++14 -Wextra  -fno-exceptions -fno-rtti -MMD -MP -Wl,-stack,10485760
#gcc 4.2以降、-march=nativeのオプションを与えると、CPUに合わせた最適化オプションが得られるようになった
CFLAGS   += -march=native 
#CFLAGS +=	-g -O0
#CFLAGS+=-O3

#std::thread用
LDFLAGS  = -lpthread
LIBS     =
INCLUDE  =  #-I../include
ifeq ($(OS),Windows_NT)
  TARGET = squirrel.exe
  LDFLAGS += -static#スタティックリンクを使うように指定する(dllは使わないように)
else
  TARGET = squirrel
endif
#中間ファイルなどを置くディレクトリを宣言
OBJDIR   = ./obj
#ソースファイルの宣言 ここの順番がおかしいせいでエラーが起こる？？
SOURCES := $(wildcard *.cpp)
HEADER := $(wildcard *.h)
#addprefixで OBJDIR/souces.oを指定している
OBJECTS  = $(addprefix $(OBJDIR)/, $(SOURCES:.cpp=.o))
DEPENDS  = $(OBJECTS:.o=.d)


$(TARGET): $(OBJECTS) $(LIBS)
	$(COMPILER) -o $@ $^ $(LDFLAGS) $(CFLAGS)


$(OBJDIR)/%.o: %.cpp
	$(COMPILER) $(CFLAGS) $(INCLUDE) -o $@ -c $<


clean:
	 rm ./obj/*
	 rm squirrel.exe

release:
	$(MAKE) CFLAGS='$(CFLAGS)' LDFLAGS='$(LDFLAGS) -flto' $(TARGET)


test:
	echo $(OBJECTS)

# ソースファイルの依存関係が明記された.dファイルをインクルードします。
# これによって，ヘッダファイルのみが更新された場合でもmakeが実行される事になります。
-include $(DEPENDS)
