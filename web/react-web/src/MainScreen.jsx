import React, { Component, useState } from 'react'
import Diretorio from './Diretorio'
import Pastas from './Pastas'
import './MainScreen.css'

export default props => {


	const pastas = React.createRef();
	const [path, changePath] = useState("/");

	function onChangePath(newPath){
		pastas.current.clean();
		changePath(newPath);
		pastas.current.listar();
	}	

	return (
		<div className="MainScreen">
			<Diretorio path={path} onChangePath={onChangePath} />
			<Pastas path={path} ref={pastas}/>
		</div>
	)


}
 