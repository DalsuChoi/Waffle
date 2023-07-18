import React, { useEffect, useState } from "react"
import './index.css'

function AdditionalInfo(props) {

	const [objects, setObjects] = useState(0);
	const [queries, setQueries] = useState(0);
	const [regrids, setRegrids] = useState(0);
	
	
	function setAdditionalInfo(i0, i1, i2) {
    	setObjects(i0);
    	setQueries(i1);
    	setRegrids(i2);
    }

	useEffect(() => {
		const intervalId = setInterval(() => {
			props.handler(setAdditionalInfo);
		}, 1000);

		return () => clearInterval(intervalId);
	}, [props]);

  	return (
			<div className="dashboard-03">
				<div className="num-objects">
					<h2> The number of objects in the space :</h2>
					<input 
						type="text" 
						className="input-height-40"
						value={objects} />
				</div>
				<div className="performed-regrids">
					<h2>Performed regrids :</h2>
					<input 
						type="text" 
						className="input-height-40"
						value={regrids} />
				</div>
				<div className="processed-user-queries">
					<h2>Processed user queries :</h2>
					<input 
						type="text" 
						className="input-height-40"
						value={queries} />
				</div>
			</div>

  	);
}


export default AdditionalInfo;
