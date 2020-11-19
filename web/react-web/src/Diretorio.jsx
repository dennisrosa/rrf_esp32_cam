import React, { Component } from 'react'

export default class Diretorio extends Component {

    endpoint = "http://192.168.1.70/lista?path=/";
    state = {
        contacts: []
    }
    
    listar() {
        console.log(this.endpoint);
        fetch(this.endpoint)
            .then(res => res.json())
            .then((data) => {
                console.log("res data", data);
                this.setState({ contacts: Array.from(data.path) })
            })
            .catch(console.log)
    };


    componentDidMount() {
        this.listar();
    }

    render() {
        return (
            <div>
                <div className="Diretorio" >
                    <h1>Diretorios {this.props.path} </h1>
                    <ul>
                        {this.state.contacts.map((c) => (
                            <li key={c.path} >
                                {c.directory === '1' ? <a  onClick={() => {this.props.onChangePath(c.path)} }>{c.path}</a> : c.path}
                            </li>
                        ))}
                    </ul>
                </div>

            </div>
        )
    }    

}


