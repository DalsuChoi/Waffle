import { React, useRef } from "react";
import SingleGraph from "./SingleGraph";
import './index.css'

function GraphWrapper(props) {

	const rewardRef = useRef();
	const lossRef = useRef();
	const insertionRef = useRef();
	const deletionRef = useRef();
	const rangeRef = useRef();
	const knnRef = useRef();
	const memoryRef = useRef();

	setInterval(() => {
		const { current: rewardGraph } = rewardRef;
		const { current: lossGraph } = lossRef;
		const { current: insertionGraph } = insertionRef;
		const { current: deletionGraph } = deletionRef;
		const { current: rangeGraph } = rangeRef;
		const { current: knnGraph } = knnRef;
		const { current: memoryGraph } = memoryRef;
		
		props.handler(rewardGraph, lossGraph, insertionGraph, deletionGraph, rangeGraph, knnGraph, memoryGraph);

		rewardGraph.update();
		lossGraph.update();
		insertionGraph.update();
		deletionGraph.update();
		rangeGraph.update();
		knnGraph.update();
		memoryGraph.update();
    }, 5000);
  	
	return (
			<>
			<div className="reward-chart">
				<h3 className="h3-reward">Reward</h3>
				<SingleGraph ref={rewardRef} title="Reward"/>
			</div>
			<div className="loss-chart">
				<h3 className="h3-loss">Loss</h3>
				<SingleGraph ref={lossRef} title="Loss" />
			</div>
			<div className="charts">
				<div className="insertion-chart">
					<h3 className="h3-insertion">Insertion Query</h3>
					<SingleGraph ref={insertionRef} title="Insertion Query" />
				</div>
				<div className="deletion-chart">
					<h3 className="h3-deletion">Deletion Query</h3>
					<SingleGraph ref={deletionRef} title="Deletion Query" />
				</div>
				<div className="range-chart">
					<h3 className="h3-range">Range Query</h3>
					<SingleGraph ref={rangeRef} title="Range Query" />
				</div>
				<div className="knn-chart">
					<h3 className="h3-knn">k-NN Query</h3>
					<SingleGraph ref={knnRef} title="k-NN Query" />
				</div>
				<div className="memory-chart">
					<h3 className="h3-memory">Memory Usage</h3>
					<SingleGraph ref={memoryRef} title="Memory Usage" />
				</div>
			</div>

			</>
  	);
}

export default GraphWrapper;
