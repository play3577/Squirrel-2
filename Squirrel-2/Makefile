#makefile������ā@g++�ł̃r���h�ɒ���I�I
#�S�R�킩���(�O���O)

#�R���p�C���̎w��
COMPILER = g++
#�R���p�C�����̃t���O
CFLAGS�@= -std=c++14 -Wextra -Ofast 
#���s�t�@�C����
TARGET= squirrel
#objectfile�̍쐬�ʒu
OBJDIR= ../obj 
#SOURCE��HEADER��OBJECTS
SOURCES := $(wildcard *.cpp)
HEADER := $(wildcard *.h)
OBJECTS  = $(addprefix $(OBJDIR)/, $(SOURCES:.cpp=.o))
DEPENDS  = $(OBJECTS:.o=.d)

#target�������ŗ^����ꂽ�i�܂��͈��������������Ȃ������j�Ƃ��ɉ������邩
$(TARGET): $(OBJECTS)
	$(COMPILER) -o $@ $^ $(CFLAGS)


#�I�u�W�F�N�g�t�@�C���̍쐬
$(OBJDIR)/%.o: %.cpp
	#@[ -d $(OBJDIR) ] || mkdir -p $(OBJDIR)
	$(COMPILER) $(CFLAGS)  -o $@ -c $<

test:
	echo $(TARGET)

debug:
	$(MAKE) CFLAGS='$(CFLAGS) -D_DEBUG -flto' $(TARGET)

release:
	$(MAKE) CFLAGS='$(CFLAGS) -flto' $(TARGET)
