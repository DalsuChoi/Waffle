class WaffleApi {

    constructor(serviceAddress) {
        this.serviceAddress = serviceAddress;
    }
    
    makeURL_dummy(operation) {
    	const resource = operation + "/0/0/0";
        return new URL(resource, this.serviceAddress);
    }

	makeURL_setUserParameters(p0, p1, p2) {
		const resource = "set_user_parameters/" + p0 + "/" + p1 + "/" + p2;
		return new URL(resource, this.serviceAddress);
	}
	setUserParameters(p0, p1, p2) {
		fetch(this.makeURL_setUserParameters(p0, p1, p2));
	}

    tryButton(setHyperparameters) {
        fetch(this.makeURL_dummy("try"))
        .then(response => response.json())
        .then((response) => {
        	setHyperparameters(response["lr"], response["batch_size"], response["candidates"], response["T"], response["recent"]);
            }, (err) => {
                console.log(err);
            }
        );
    }

    evaluateButton() {
        fetch(this.makeURL_dummy("evaluate"));
    }

    bestButton(setHyperparameters) {
        fetch(this.makeURL_dummy("best"))
        .then(response => response.json())
        .then((response) => {
        	setHyperparameters(response["lr"], response["batch_size"], response["candidates"], response["T"], response["recent"]);
            }, (err)=> {
                console.log(err);
            }
        );
    }

    convergence() {
    	fetch(this.makeURL_dummy("convergence"));
    }

    currentKnobSetting(setKnobSetting) {
        fetch(this.makeURL_dummy("knobs"))
        .then(response => response.json())
        .then((response) => {
        	setKnobSetting(response["latSpace"], response["lonSpace"], response["maxObjectsPerCell"], response["latChunk"], response["lonChunk"]);
        	return response;
        },
            (err)=> {
                console.log(err);
        });
    }

    additionalInfo(setAdditionalInfo) {
        fetch(this.makeURL_dummy("info"))
        .then(response => response.json())
        .then((response)=> {
        	setAdditionalInfo(response["objects"], response["queries"], response["regrids"]);
        },
            (err)=> {
                console.log(err);
        });
    }

    push_new_data(graph, x, y) {
    	if(Number(y) !== 0) {
    		graph.data.labels.push(Number(x));
        	graph.data.datasets[0].data.push(Number(y));
        }
    }
    newStatistics(rewardGraph, lossGraph, insertionGraph, deletionGraph, rangeGraph, knnGraph, memoryGraph) {
    	fetch(this.makeURL_dummy("statistics"))
    	.then(response => response.json())
        .then((response)=> {
        	const episode = response["episode"];
        	this.push_new_data(rewardGraph, episode, response["reward"]);
            this.push_new_data(lossGraph, episode, response["loss"]);
        	this.push_new_data(insertionGraph, episode, response["insertion"]);
        	this.push_new_data(deletionGraph, episode, response["deletion"]);
        	this.push_new_data(rangeGraph, episode, response["range"]);
        	this.push_new_data(knnGraph, episode, response["knn"]);
        	this.push_new_data(memoryGraph, episode, response["memory"]);
          },
          (err)=> {
            console.log(err);
          }
        );
    }
}

export default WaffleApi;
