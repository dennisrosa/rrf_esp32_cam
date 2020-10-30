import React, { Component, useState } from 'react'
import Diretorio from './Diretorio'
import Pastas from './Pastas'
import './MainScreen.css'

export default props => {

	const [nome, alteraNome] = useState('Pedro')

	return (
		<div className="MainScreen">
			<Diretorio path={nome} />
			<Pastas path={nome} />
		</div>
	)


}
 