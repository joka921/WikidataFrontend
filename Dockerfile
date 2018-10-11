FROM ubuntu:18.04 as base
MAINTAINER Johannes Kalmbach <johannes.kalmbach@gmail.com>
ENV LANG C.UTF-8
ENV LC_ALL C.UTF-8
ENV LC_CTYPE C.UTF-8

FROM base as builder
RUN apt-get update && apt-get install -y build-essential cmake libboost-system-dev libboost-serialization-dev libcurl4-openssl-dev
COPY . /app/


WORKDIR /app/build/
RUN cmake -DCMAKE_BUILD_TYPE=Release .. && make -j $(nproc)

FROM base as runtime
WORKDIR /app
RUN apt-get update && apt-get install -y libboost-system-dev libboost-serialization-dev libcurl4-openssl-dev
ARG UID=1000
RUN groupadd -r wikidata-frontend && useradd --no-log-init -r -u $UID -g wikidata-frontend wikidata-frontend && chown wikidata-frontend:wikidata-frontend /app
#RUN apt-get update && apt-get install -y bzip2

COPY --from=builder /app/build/*Main /app/src/web/* /app/
ENV PATH=/app/:$PATH

USER wikidata-frontend
EXPOSE 7001
VOLUME ["/input"]

ENV INPUT_PREFIX input
ENV QLEVER_ADDRESS panarea.informatik.privat
ENV QLEVER_PORT 7001
# Need the shell to get the INPUT_PREFIX envirionment variable
ENTRYPOINT ["/bin/sh", "-c", "exec WikidataFrontendMain \"/input/${INPUT_PREFIX}\" 7001 ${QLEVER_ADDRESS} ${QLEVER_PORT}"]

# docker build -t wikidata-frontend-<name> .
# # When running with user namespaces you may need to make the input folder accessible
# # to e.g. the "nobody" user
# chmod -R o+rw ./input

# # You need to have input files called input.entities and input.desc in the ./input Volume.
# # If you want to use a different prefix for those files,
# # set the environment variable "INPUT_PREFIX" during `docker run` using `-e INPUT_PREFIX=<prefix>`
# # (default value is "input")

# # To run a server use
# docker run -d -p 7001:7001 -e "INPUT_PREFIX=<prefix>" -v "$(pwd)/input:/input" -e "QLEVER_ADDRESS=<ip-of-qlever-server> -e "QLEVER_PORT=<port-of-qlever-server"--name wikidata-frontend-<name> wikidata-frontend-<name>

