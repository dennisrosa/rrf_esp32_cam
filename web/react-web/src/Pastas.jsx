import React, { Component } from 'react'

export default class Pastas extends Component {

    imgEndpoint = "/image?path=";
    endpoint = "/lista?path=";
	
    state = {
        contacts: [],
        loaded: 0,
    }

    clean() {
        this.setState({ loaded: 0 })
        this.setState({ contacts: [] })
        document.getElementById('imgGif').innerHTML = "";
    };

    componentDidMount() {
        this.myInterval = setInterval(() => {

            if(this.state.loaded  <= this.state.contacts.length  ){
                this.setState({ loaded: this.imagesLoaded() })
            }
             
        }, 1000)
    }

    listar() {
		const url = this.endpoint + this.props.path
        this.setState({ contacts: ['a'] });

		console.log("Path" , url)
        fetch(url, { mode: "no-cors" })
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

        var animatedImage = document.createElement('img');
        ag.getBase64GIF(function (image) {
            animatedImage.src = image;
            var e = document.getElementById('imgGif');
            e.appendChild(animatedImage);
        });
        console.log(imgs);
    }

    imagesLoaded() {
        var count = 0;
        const imgElements = document.querySelectorAll('img');
        for (let i = 0; i < imgElements.length; i += 1) {
          const img = imgElements[i];
          if (img.complete) {
            count++;
          }
        }
        return count;
      }

    onFallbackImage(ev){
        console.log("fallback image"  +ev.target.src );
        ev.target.src = ev.target.src + "&a=1"
    }

    display(){

        var text = this.props.path ;
        if (this.state.contacts.length === 1){
            text +=  " - loading ....  " ;            
        } else if (this.state.loaded  != this.state.contacts.length){
            text +=  " - Processing ....  " + this.state.loaded + " of " + (this.state.contacts.length +1) ;            
        }
        return text;
    }

    render() {

        return (
            <div>
                <div className="Pasta">
                    <h1>Pastas {this.display()}</h1>
                    { this.state.contacts.length > 0 & this.state.loaded === this.state.contacts.length ? <button onClick={() => this.gerarGif()}>Gerar GIF</button> : null}
        
                    <div id="imgGif"></div>
                    
                    <ul>
                        {this.state.contacts.map((c) => (
                            <li key={c.path} >
                                {c.directory === '0' ? <img onError={this.onFallbackImage}  crossOrigin="Anonymous" src={ this.imgEndpoint + encodeURIComponent(c.path) } /> : <div></div>}
                            </li>
                        ))}
                    </ul>
                </div>
            </div>
        )
    }
}
