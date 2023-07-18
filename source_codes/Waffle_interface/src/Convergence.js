import React, { useState } from 'react';
import './index.css'

function Convergence(props) {
  const [buttonClicked, setButtonClicked] = useState(false);

  function handleClick() {
    setButtonClicked(!buttonClicked);
    props.convergenceClicked();
  }

  return (
    <div>
      <input 
        type="button"
        className={props._convergeButton? "stop-training-button button-on" :
                                "stop-training-button button-off"}
        disabled={!props._convergeButton}
        value={buttonClicked ? "Restart training" : "Stop training"}
        onClick={handleClick}
      />
    </div>
  );
}

export default Convergence;
