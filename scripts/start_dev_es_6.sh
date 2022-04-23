docker run --rm -it --name "sist2-dev-es-6"\
       	-p 9202:9200 -e "discovery.type=single-node" \
	-e "ES_JAVA_OPTS=-Xms8g -Xmx8g" elasticsearch:6.8.0
