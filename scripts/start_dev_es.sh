docker run --rm -it --name "sist2-dev-es"\
       	-p 9200:9200 -e "discovery.type=single-node" \
	-e "ES_JAVA_OPTS=-Xms8g -Xmx8g" elasticsearch:7.14.0
