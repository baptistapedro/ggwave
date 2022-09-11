FROM fuzzers/afl:2.52

RUN apt-get update
RUN apt install -y build-essential wget git clang cmake  automake autotools-dev  libtool zlib1g zlib1g-dev libexif-dev libjpeg-dev libpng-dev libsdl2-dev
RUN git clone --recursive https://github.com/ggerganov/ggwave.git
WORKDIR /ggwave
RUN cmake -DCMAKE_C_COMPILER=afl-clang -DCMAKE_CXX_COMPILER=afl-clang++ .
RUN make
RUN make install
RUN mkdir /ggwaveCorpus
#RUN wget https://www2.cs.uic.edu/~i101/SoundFiles/CantinaBand60.wav
#RUN wget https://www2.cs.uic.edu/~i101/SoundFiles/Fanfare60.wav
RUN wget https://www2.cs.uic.edu/~i101/SoundFiles/preamble10.wav
RUN wget https://www2.cs.uic.edu/~i101/SoundFiles/StarWars3.wav
RUN wget https://www2.cs.uic.edu/~i101/SoundFiles/taunt.wav
RUN cp *.wav /ggwaveCorpus


ENTRYPOINT ["afl-fuzz", "-t", "3000+", "-i", "/ggwaveCorpus", "-o", "/ggwaveOut"]
CMD  ["/ggwave/bin/ggwave-from-file", "@@"]
