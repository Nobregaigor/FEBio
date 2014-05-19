include ../../make.$(PLAT)

SRC = $(wildcard $(FEBDIR)FEBio2/*.cpp)
OBJ = $(patsubst $(FEBDIR)FEBio2/%.cpp, %.o, $(SRC))
DEP = $(patsubst $(FEBDIR)FEBio2/%.cpp, %.d, $(SRC))

TARGET =  $(FEBDIR)bin/febio2.$(PLAT)

FELIBS =  $(FEBDIR)lib/libfecore_$(PLAT).a
FELIBS += $(FEBDIR)lib/libfebiolib_$(PLAT).a
FELIBS += $(FEBDIR)lib/libfebioplot_$(PLAT).a
FELIBS += $(FEBDIR)lib/libfebiomech_$(PLAT).a
FELIBS += $(FEBDIR)lib/libfebiomix_$(PLAT).a
FELIBS += $(FEBDIR)lib/libfebioheat_$(PLAT).a
FELIBS += $(FEBDIR)lib/libfebioxml_$(PLAT).a
FELIBS += $(FEBDIR)lib/libnumcore_$(PLAT).a
FELIBS += $(FEBDIR)lib/libfebioopt_$(PLAT).a

FEBIOLIBS = -Wl,--start-group $(FELIBS) -Wl,--end-group

$(TARGET): $(OBJ) $(FELIBS)
ifeq ($(findstring lnx,$(PLAT)),lnx)
	$(CC) -o $(TARGET) $(DEF) $(FLG) $(INC) $(OBJ) $(FEBIOLIBS) $(LIBS) -ldl
else ifeq ($(findstring gcc,$(PLAT)),gcc)
	$(CC) -o $(TARGET) $(DEF) $(FLG) $(INC) $(OBJ) $(FEBIOLIBS) $(LIBS) -ldl
else ifeq ($(findstring sky,$(PLAT)),sky)
	$(CC) -o $(TARGET) $(DEF) $(FLG) $(INC) $(OBJ) $(FEBIOLIBS) $(LIBS) -ldl
else
	$(CC) -o $(TARGET) $(DEF) $(FLG) $(INC) $(OBJ) $(FELIBS) $(LIBS)
endif

%.o: $(FEBDIR)FEBio2/%.cpp
	$(CC) $(INC) $(DEF) $(FLG) -MMD -c $<

clean:
	$(RM) *.o *.d $(TARGET)

-include $(DEP)