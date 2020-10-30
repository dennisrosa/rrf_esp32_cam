import React, { Component } from 'react'

export default class Pastas extends Component {

    imgEndpoint = "/xupa?path=";

    endpoint = "/lista";
    path = "";

    state = {
        contacts: []
    }

    listar(path) {
        this.path = path;
        fetch(this.endpoint + path, { mode: "no-cors" })
            .then(res => res.json())
            .then((data) => {
                this.setState({ contacts: Array.from(data.path) })
            })
            .catch(console.log)
    };

    gerarGif() {
        console.log("Gerar");
        var imgs = document.querySelectorAll('img');

        var ag = new window.Animated_GIF();
        ag.setSize(640, 480);
        ag.setDelay(1);



        for (var i = 0; i < imgs.length; i++) {
            ag.addFrame(imgs[i]);
        }


        // This is asynchronous, rendered with WebWorkers
        var animatedImage = document.createElement('img');
        ag.getBase64GIF(function (image) {
            animatedImage.src = image;
            var e = document.getElementById('imgGif');
            e.appendChild(animatedImage);
        });

        console.log(imgs);
    }


    componentDidMount() {
        this.listar("/");
    }

    render() {
        return (
            <div>
                <div className="Pasta">
                    <h1>Pastas  {this.props.path} </h1>
                    <div id="imgGif"></div>

                    {this.path !== '/' ? <button onClick={() => this.gerarGif()}>Gerar GIF</button> : null}
                    <ul>
                        {this.state.contacts.map((c) => (
                            <li key={c.path} >
                                {c.directory === '0' ? <img crossOrigin="Anonymous" src={this.imgEndpoint + c.path} /> : <div></div>}
                            </li>
                        ))}
                    </ul>

                </div>

            </div>
        )
    }
}

