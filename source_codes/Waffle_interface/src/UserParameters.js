import React, { useState } from "react";
import OverlayTrigger from "react-bootstrap/OverlayTrigger";
import Tooltip from "react-bootstrap/Tooltip";

function UserParameters(props) {

	const [wTime, setWTime] = useState('');
	const [wMemory, setWMemory] = useState('');
	const [every, setEvery] = useState('');

	const [setButton, setSetButton] = useState(true);

	const createTooltip = (props, text) => (
		<Tooltip {...props} className="tooltip">
			{text}
		</Tooltip>
	);

	function setClick() {
		setSetButton(false);
        props._setTryButton(true);
		props.setUserParametersClicked(wTime, wMemory, every);
	}

	const w_time_tooltip = props => createTooltip(props, 'A trade-off between query processing times and memory usage (time weight + memory weight = 1)');
	const w_memory_tooltip = props => createTooltip(props, 'A trade-off between query processing times and memory usage (time weight + memory weight = 1)');
	
  	return (
    		<div className={setButton ? "dashboard-01" : "dashboard-01 dashboard-01-clicked"}>
				<div className="parameters-title">
					<h1 className={setButton ? "" : "h-clicked"}>User Preference</h1>
				</div>

				<div className="sec-01">
					<div className="w-time">
						<OverlayTrigger overlay={w_time_tooltip}>
							<h2 className={setButton ? "" : "h-clicked"}>w<sub>time</sub> :</h2>
      					</OverlayTrigger>
						<input
							type="text"
							className="input-height-32"
							name="w_time"
							value={wTime}
							onChange={e => setWTime(e.target.value)}
							disabled={!setButton} />
					</div>

					<div className="w-memory">
						<OverlayTrigger overlay={w_memory_tooltip}>
							<h2 className={setButton ? "" : "h-clicked"}>w<sub>memory</sub> :</h2>
						</OverlayTrigger>
						<input
							type="text"
							className="input-height-32"
							name="w_memory"
							value={wMemory}
							onChange={e => setWMemory(e.target.value)}
							disabled={!setButton} />
					</div>
					
					<div className="every">
						<h2 className={setButton ? "" : "h-clicked"}>Perform a new regrid</h2>
						<input
							type="text"
							className="input-height-32"
							name="every"
							value={every}
							onChange={e => setEvery(e.target.value)}
							disabled={!setButton} />
						<h2 className={setButton ? "" : "h-clicked"}> seconds after the previous regrid</h2>
					</div>	
					
					<div>
						<input 
							type="button"
							className={setButton ? "set-button button-on" : "set-button button-off"}
							value="Set"
							onClick={setClick}
							/>
					</div>
				</div>		
			</div>
  	);
}


export default UserParameters;