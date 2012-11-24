CFLAGS=-Wall -g

BIN=./BIN
SOURCE=./

all: $(BIN)/server

$(BIN)/%: $(SOURCE)%.c
	$(CC) $(INC) $< $(CFLAGS) -o $@ $(LIBS)

clean:
	rm -r $(BIN)/server
