all: search cut

server: src/search_engine_server.o src/recommand_words.o src/words_cut.o src/inverted_index.o src/shortest.o src/return_title.o src/LRU.o src/INIReader.o src/ini.o 
	g++ src/search_engine_server.o src/recommand_words.o src/words_cut.o src/inverted_index.o src/shortest.o src/return_title.o src/LRU.o src/INIReader.o src/ini.o -o bin/serve -lredis++ -lhiredis -pthread -lworkflow -lwfrest 

src/search_engine_server.o: src/search_engine_server.cpp
	g++ -O2 -g -c src/search_engine_server.cpp -o src/search_engine_server.o

search: main.cpp src/main.o src/shortest.o src/recommand_words.o src/INIReader.o src/ini.o
	g++ src/main.o src/shortest.o src/ini.o src/INIReader.o src/recommand_words.o -o bin/Search -lredis++ -lhiredis -pthread

cut: src/words_cut.o src/INIReader.o src/ini.o
	g++ src/words_cut.o src/INIReader.o src/ini.o -o bin/cut -lredis++ -lhiredis -pthread

xml: src/rss2.o src/article_manager.o src/words_cut.o src/inverted_index.o src/tinyxml2.o src/INIReader.o src/ini.o 
	g++ src/rss2.o src/article_manager.o src/words_cut.o src/inverted_index.o src/tinyxml2.o src/INIReader.o src/ini.o -o bin/xml -lredis++ -lhiredis -pthread

src/return_title.o: src/returnTitle.cpp
	g++ -O2 -g -c src/returnTitle.cpp -o src/return_title.o

src/inverted_index.o: src/inverted_index.cpp
	g++ -O2 -g -c src/inverted_index.cpp -o src/inverted_index.o

src/article_manager.o: src/article_manager.cpp 
	g++ -O2 -g -c src/article_manager.cpp -o src/article_manager.o

src/rss2.o: src/rss2.cc
	g++ -O2 -g -c src/rss2.cc -o src/rss2.o

src/tinyxml2.o: src/tinyxml2.cc
	g++ -O2 -c src/tinyxml2.cc -o src/tinyxml2.o

src/LRU.o: src/LRU.cpp
	g++ -O2 -c src/LRU.cpp -o src/LRU.o
src/words_cut.o: src/words_cut.cpp
	g++ -O2 -g -c src/words_cut.cpp -o src/words_cut.o

src/main.o: main.cpp
	g++ -O2 -c main.cpp -o src/main.o

src/recommand_words.o: src/recommand_words.cpp 
	g++ -O2 -c src/recommand_words.cpp  -o src/recommand_words.o -lredis++ -lhiredis -pthread

src/shortest.o: src/shortest.cpp
	g++ -O2 -c src/shortest.cpp -o src/shortest.o

src/INIReader.o: src/INIReader.cpp
	g++ -c src/INIReader.cpp -o src/INIReader.o

src/ini.o: src/ini.c
	g++ -c src/ini.c -o src/ini.o


clean:
	rm -f bin/*
	rm -f src/*.o

cleanXml:
	rm -f data/offsetLib.dat
	rm -f data/ripepage.dat
	rm -f data/web_page_contents.dat

cleanDict:
	rm -f data/dict.dat


.PHONY: all clean
