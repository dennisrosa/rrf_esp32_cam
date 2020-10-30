import React, { Component } from 'react'

export default class Diretorio extends Component {

    endpoint = "/lista";
    path = "";

    state = {
        contacts: []
    }
    
    listar(path) {
        this.path = path;
        fetch(this.endpoint, { mode: "no-cors" })
            .then(res => res.json())
            .then((data) => {
                this.setState({ contacts: Array.from(data.path) })
            })
            .catch(console.log)
    };


    componentDidMount() {
        this.listar("/");
    }

    render() {
        return (
            <div>
                <div className="Diretorio" >
                    <h1>Diretorios {this.props.path} </h1>
                    <ul>
                        {this.state.contacts.map((c) => (
                            <li key={c.path} >
                                {c.directory === '1' ? <a href="#" onClick={() => {this.props.onChangePath(c.path)} }>{c.path}</a> : c.path}
                            </li>
                        ))}
                    </ul>
                </div>

            </div>
        )
    }    


}


