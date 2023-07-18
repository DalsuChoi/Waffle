import {React, useState, useRef } from "react";
import OverlayTrigger from "react-bootstrap/OverlayTrigger";
import Tooltip from "react-bootstrap/Tooltip";

function HyperParameters(props) {

	const [learningRate, setLearningRate] = useState('');
	const [batchSize, setBatchSize] = useState('');
	const [candidates, setCandidates] = useState('');
	const [t, setT] = useState('');
	const [recent, setRecent] = useState('');

	const [internalTryButton, setInternalTryButton] = useState(false);
	const [evaluateButton, setEvaluateButton] = useState(false);
	const [bestButton, setBestButton] = useState(false);

	const explanation = useRef();
	function setExplanation(newText) {
		explanation.current.textContent = newText;
	}
	
	const createTooltip = (props, text) => (
		<Tooltip {...props} className="tooltip">
			{text}
		</Tooltip>
	);

	function setHyperparameters(lr, batch, candidates, t, recent) {
    	setLearningRate(lr);
    	setBatchSize(batch);
    	setCandidates(candidates);
    	setT(t);
    	setRecent(recent);
    }

	function tryClick() {
		props._setTryButton(false);
		setInternalTryButton(false);
		setEvaluateButton(true);
		setBestButton(false);
		props.tryButtonClicked(setHyperparameters);
		props._setConvergeButton(false);
		setExplanation("Trying new hyperparameter values");
	}

	function evaluateClick() {
		setInternalTryButton(true);
		setEvaluateButton(false);
		setBestButton(true);
		props.evaluateButtonClicked();
		setExplanation("Evaluating the hyperparameter values");
	}

	function bestClick() {
		props._setTryButton(true);
		setInternalTryButton(true);
		setEvaluateButton(false);
		setBestButton(false);
		props.bestButtonClicked(setHyperparameters);
		props._setConvergeButton(true);
		setExplanation("The best hyperparameter values among the trials");
	}

	const batch_size_tooltip = props => createTooltip(props, 'Batch size when updating a WaffleMaker model');
	const learning_rate_tooltip = props => createTooltip(props, 'Learning rate when updating a WaffleMaker model');
	const candidates_tooltip = props => createTooltip(props, '|candidates| for exploration');
	const recent_tooltip = props => createTooltip(props, '|recent| for exploitation');
	const t_tooltip = props => createTooltip(props, 'T for exploration');

  	return (
    		<div className="hyperparameter">
				<div className="parameters-title">
					<h1>Hyperparameters</h1>
				</div>
	
				<div className="learning-rate">
					<OverlayTrigger overlay={learning_rate_tooltip}>
						<h2>Learning rate :</h2>
					</OverlayTrigger>						
					<input
						type="text"
						className="input-height-32"
						name="learning_rate"
						value={learningRate} />
				</div>

				<div className="batch-size">
					<OverlayTrigger overlay={batch_size_tooltip}>
						<h2>Batch size :</h2>
					</OverlayTrigger>
					<input
						type="text"
						className="input-height-32"
						name="batch_size"
						value={batchSize} />
				</div>		

				
				<div className="candidates">
					<OverlayTrigger overlay={candidates_tooltip}>
						<h2>|candidates| :</h2>
					</OverlayTrigger>
					<input
						type="text"
						className="input-height-32"
						name="candidates"
						value={candidates} />
				</div>


				<div className="t">
					<OverlayTrigger overlay={t_tooltip}>
						<h2>T :</h2>
					</OverlayTrigger>
					<input
						type="text"
						className="input-height-32"
						name="t"
						value={t} />
				</div>		
				
				<div className="recent">
					<OverlayTrigger overlay={recent_tooltip}>
						<h2>|recent| :</h2>
					</OverlayTrigger>
					<input
						type="text"
						className="input-height-32"
						name="recent"
						value={recent} />
				</div>
					
				<div className="explanation" ref={explanation}>
					
				</div>

				<div>
					<input
						type="button"
						className={(props._tryButton || internalTryButton) ? 
							"try-button button-on": "try-button button-off"}
						disabled={(!props._tryButton && !internalTryButton)}
						value="Try"
        				onClick={tryClick}
						/>
				</div>

				<div>
					<input 
						type="button"
						className={evaluateButton ? 
							"evaluate-button button-on" : "evaluate-button button-off"}
						disabled={!evaluateButton}
						value="Evaluate"
        				onClick={evaluateClick} />
				</div>

				<div>
					<input
						type="button"
						className={bestButton ? 
							"best-button button-on" : "best-button button-off"}
						disabled={!bestButton}
						value="Best"
        				onClick={bestClick} />
				</div>
			</div>
  	);
}


export default HyperParameters;
