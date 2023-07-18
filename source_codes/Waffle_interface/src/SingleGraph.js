import { Line } from "react-chartjs-2";
import { React, forwardRef } from "react";

import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend
} from "chart.js";

ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend
);

function SingleGraph(props, ref) {

	let color;
	let xlabel;
	let ylabel;

	switch (props.title) {
		case "Reward":
			color = "#0788FF";
			xlabel = "Episode";
			ylabel = "Reward";
			break;
		case "Loss":
			color = "#F6747C";
			xlabel = "Episode";
			ylabel = "Loss";
			break;
		case "Insertion Query":
			color = "#FFFFFF";
			xlabel = "Episode";
			ylabel = "Time (ns)";
			break;
		case "Deletion Query":
			color = "#FFFFFF";
			xlabel = "Episode";
			ylabel = "Time (ns)";
			break;
		case "k-NN Query":
			color = "#FFFFFF";
			xlabel = "Episode";
			ylabel = "Time (ns)";
			break;	
		case "Range Query":
			color = "#FFFFFF";
			xlabel = "Episode";
			ylabel = "Time (ns)";
			break;	
		case "Memory Usage":
			color = "#FFFFFF";
			xlabel = "Episode";
			ylabel = "Usage (MB)";
			break;						
	  	default:
			color = "#FFFFFF";
			break;
	}
	
	const chartData = {
		labels: [],
		datasets: [{
			data: [],
		    backgroundColor: "#FFFFFF",
		    borderColor: color,
			borderWidth: 1,
			pointRadius: 2.5
		}]
	}

	const options = {
		scales: {
			x: {
				type: 'linear', 
				title: {
					display: true,
					text: xlabel, 
					color: "#FFFFFF"
				},
				ticks: {
					fontColor: "#FFFFFF",
					fontSize: '8px',
					position: "outside",
					stepSize: 1
				},
				gridLines: {
					zeroLineColor: '#505050',
					color: '#505050',
					thickness: 5
				}
			},
			y: {
				type: 'linear', 
				title: {
					display: true,
					color: "#FFFFFF",
					fontSize: '12px',
					text: ylabel, 
				},
				ticks: {
					fontColor: "#FFFFFF",
					fontSize: '12px',
					position: "outside",
				},
				gridLines: {
					zeroLineColor: '#505050',
					color: '#505050',
					thickness: 5
				}
			}
		},
		
		plugins: {
			title: {
				display: false
			},
			legend: {
				display: false
			}
		}
	}

	return (
		<div className="single-graph">
			<Line
				ref={ref}
				data={chartData}
				options={options}
			/>
		</div>
	);
}

export default forwardRef(SingleGraph);
