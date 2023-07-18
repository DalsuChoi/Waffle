import React, { useEffect, useState } from "react"
import './index.css'

function CurrentKnobSetting(props) {
	const [latSpace, setLatSpace] = useState(0);
	const [lonSpace, setLonSpace] = useState(0);
	const [maxObjectsPerCell, setMaxObjectsPerCell] = useState(0);
	const [latChunk, setLatChunk] = useState(0);
	const [lonChunk, setLonChunk] = useState(0);

	function setKnobSetting(k0, k1, k2, k3, k4) {
    	setLatSpace(k0);
    	setLonSpace(k1);
    	setMaxObjectsPerCell(k2);
    	setLatChunk(k3);
    	setLonChunk(k4);
    }

	useEffect(() => {
		const intervalId = setInterval(() => {
			props.handler(setKnobSetting);
		}, 1000);

		return () => clearInterval(intervalId);
	}, [props]);

  	return (
			<div className="dashboard-02">
				<div className="current-knob-setting-title">
					<h1>Current Knob Setting</h1>
				</div>
				<div className="current-knob-setting-frame">
					<div className="lat-space">
						<h2>The number of cells along latitude in the space :</h2>
						<input 
							type="text" 
							className="input-height-40"
							value={latSpace} />
					</div>	
					<div className="lon-space">
						<h2>The number of cells along longitude in the space :</h2>
						<input 
							type="text" 
							className="input-height-40"
							value={lonSpace} />
					</div>											
					<div className="max-objects-per-cell">
						<h2>Maximum Objects Per Cell :</h2>
						<input 
							type="text" 
							className="input-height-40"
							value={maxObjectsPerCell} />
					</div>
					<div className="lat-chunk">
						<h2>The number of cells along latitude in a single chunk :</h2>
						<input 
							type="text" 
							className="input-height-40"
							value={latChunk} />
					</div>	
					<div className="lon-chunk">
						<h2>The number of cells along longitude in a single chunk :</h2>
						<input 
							type="text" 
							className="input-height-40"
							value={lonChunk} />
					</div>	
				</div>
			</div>

  	);
}


export default CurrentKnobSetting;
