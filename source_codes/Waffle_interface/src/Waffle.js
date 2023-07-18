import React from 'react'
import Convergence from './Convergence'
import GraphWrapper from "./GraphWrapper";
import UserParameters from './UserParameters';
import HyperParameters from './HyperParameters';
import CurrentKnobSetting from './CurrentKnobSetting';
import AdditionalInfo from './AdditionalInfo';
import Logo from './logo';

class Waffle extends React.Component {

	constructor(props) {
		super(props);
		this.state = {
			tryButton: false,
			convergeButton: false
		};
	}

	setTryButton = (value) => {
		this.setState({ tryButton: value });
	}

	setConvergeButton = (value) => {
		this.setState({ convergeButton: value });
	}
	
	setUserParametersHandler(wTime, wMemory, x) {
		this.props.WaffleApi.setUserParameters(wTime, wMemory, x);
	}
	
	tryButtonHandler(setHyperparameters) {
		this.props.WaffleApi.tryButton(setHyperparameters);
	}

	evaluateButtonHandler() {
		this.props.WaffleApi.evaluateButton();
	}

	bestButtonHandler(setHyperparameters) {
		this.props.WaffleApi.bestButton(setHyperparameters);
	}
	
	convergenceHandler() {
		this.props.WaffleApi.convergence();
	}

	statisticsHandler(rewardGraph, lossGraph, insertionGraph, deletionGraph, rangeGraph, knnGraph, memoryGraph) {
		this.props.WaffleApi.newStatistics(rewardGraph, lossGraph, insertionGraph, deletionGraph, rangeGraph, knnGraph, memoryGraph);
	}
	
	knobsHandler(setKnobSetting) {
		this.props.WaffleApi.currentKnobSetting(setKnobSetting);
	}

	additionalInfoHandler(setAdditionalInfo){
		this.props.WaffleApi.additionalInfo(setAdditionalInfo);
	}
	
	render() {
		return (
			<div>
				<Logo />

				<UserParameters
					setUserParametersClicked={this.setUserParametersHandler.bind(this)}
					
					_tryButton={this.state.tryButton}
					_setTryButton={this.setTryButton.bind(this)}
				/>

				<HyperParameters
					tryButtonClicked={this.tryButtonHandler.bind(this)}
					evaluateButtonClicked={this.evaluateButtonHandler.bind(this)}
					bestButtonClicked={this.bestButtonHandler.bind(this)}

					_tryButton={this.state.tryButton}
					_setTryButton={this.setTryButton.bind(this)}
					_setConvergeButton={this.setConvergeButton.bind(this)}
				/>
				
				<Convergence 
					convergenceClicked={this.convergenceHandler.bind(this)}
					_convergeButton={this.state.convergeButton}
				/>

				<CurrentKnobSetting 
					handler={this.knobsHandler.bind(this)}
				/>

				<AdditionalInfo
					handler={this.additionalInfoHandler.bind(this)}
				/>
				
				<GraphWrapper 
					handler={this.statisticsHandler.bind(this)}
				/>
				
			</div>
	  	);
	}
}

export default Waffle;
