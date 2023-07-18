import React from 'react';
import './App.css';
import Waffle from './Waffle'
import WaffleApi from './WaffleApi'

function App() {
  return (
    <div className="App">
      <header className="App-header">
        <Waffle WaffleApi={new WaffleApi('http://192.168.219.135:8080')} />
      </header>
    </div>
  );
}

export default App;
