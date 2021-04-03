#!/bin/bash
for i in {1..30}
do
	mosquitto_pub -h cloud.eieiei.lol -t "dino/in" -m "Seuchenkevin $i"
done
