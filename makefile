

FLAGS = -Wall -Wextra -Wpedantic 
SERVER_SOURCES = src/Bst.c src/Phonebook.c src/Utility.c src/serverMain.c
SERVER_TARGET = Server

CLIENT_SOURCES = src/Utility.c src/clientMain.c
CLIENT_TARGET = Client

GUI_CLIENT_SOURCES = src/sGui.c src/Utility.c src/clientMain.c
GUI_CLIENT_TARGET = GuiClient

TESTER_SOURCES = src/tester.c src/Utility.c
TESTER_TARGET = Tester

server:
	gcc $(FLAGS) -pthread $(SERVER_SOURCES) -o $(SERVER_TARGET)

client:
	gcc $(FLAGS) $(CLIENT_SOURCES) -o $(CLIENT_TARGET)

guiClient:
	gcc $(FLAGS) -DUSE_GUI -Lusr/X11/lib -lX11 $(GUI_CLIENT_SOURCES) -o $(GUI_CLIENT_TARGET)

tester:
	gcc $(FLAGS) -pthread $(TESTER_SOURCES) -o $(TESTER_TARGET)

clean:
	rm $(SERVER_TARGET) $(CLIENT_TARGET) $(GUI_CLIENT_TARGET) $(TESTER_TARGET)

