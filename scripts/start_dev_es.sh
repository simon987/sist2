docker run --rm -it -p 9200:9200 -e "discovery.type=single-node" \
	-e "ES_JAVA_OPTS=-Xms8g -Xmx8g" elasticsearch:7.14.0
