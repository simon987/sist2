docker run --rm -it --name "sist2-dev-es3"\
       	-p 9200:9200 -p 9300:9300 -e "discovery.type=single-node" \
	-e "ES_JAVA_OPTS=-Xms8g -Xmx8g" elasticsearch:8.7.0
