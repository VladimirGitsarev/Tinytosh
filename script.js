const track = document.getElementById('track');

const images = [
    'img/showcase_1.jpg',
    'img/showcase_2.jpg',
    'img/showcase_3.jpg',
    'img/showcase_4.jpg',
    'img/showcase_5.jpg',
    'img/showcase_6.jpg',
    'img/showcase_7.jpg',
    'img/showcase_8.jpg',
];

const len = images.length;
let slidesHtml = '';

slidesHtml += `<div class="carousel-slide"><img src="${images[len - 2]}"></div>`;
slidesHtml += `<div class="carousel-slide"><img src="${images[len - 1]}"></div>`;

images.forEach(img => {
    slidesHtml += `<div class="carousel-slide"><img src="${img}"></div>`;
});

slidesHtml += `<div class="carousel-slide"><img src="${images[0]}"></div>`;
slidesHtml += `<div class="carousel-slide"><img src="${images[1]}"></div>`;

track.innerHTML = slidesHtml;

const slides = document.querySelectorAll('.carousel-slide');
let currentIndex = 2;
let isTransitioning = false;

function updateCarousel(instantly = false) {
    const isMobile = window.innerWidth <= 600;
    const slideWidth = isMobile ? 89 : 60; 
    const centerOffset = isMobile ? 5.5 : 20; 

    const translateVal = - (currentIndex * slideWidth) + centerOffset;

    track.style.transition = instantly ? 'none' : 'transform 0.5s ease-in-out';
    track.style.transform = `translateX(${translateVal}%)`;

    slides.forEach((slide, index) => {
        slide.style.transition = instantly ? 'none' : 'all 0.5s ease-in-out';
        slide.classList.remove('active');
        if(index === currentIndex) {
            slide.classList.add('active');
        }
    });
    
    if(instantly) {
        void track.offsetWidth; 
    }
}

updateCarousel(true);

document.getElementById('nextBtn').addEventListener('click', () => {
    if(isTransitioning) return;
    isTransitioning = true;
    currentIndex++;
    updateCarousel();
});

document.getElementById('prevBtn').addEventListener('click', () => {
    if(isTransitioning) return;
    isTransitioning = true;
    currentIndex--;
    updateCarousel();
});

track.addEventListener('transitionend', () => {
    isTransitioning = false;
    
    const totalSlides = slides.length;
    const realCount = images.length;

    if (currentIndex >= totalSlides - 2) {
        currentIndex = currentIndex - realCount; 
        updateCarousel(true);
    }
    else if (currentIndex < 2) {
        currentIndex = currentIndex + realCount; 
        updateCarousel(true);
    }
});

let touchStartX = 0;
let touchEndX = 0;

track.addEventListener('touchstart', e => {
    touchStartX = e.changedTouches[0].screenX;
});

track.addEventListener('touchend', e => {
    touchEndX = e.changedTouches[0].screenX;
    handleSwipe();
});

function handleSwipe() {
    if (touchEndX < touchStartX - 50) {
        document.getElementById('nextBtn').click();
    }
    if (touchEndX > touchStartX + 50) {
        document.getElementById('prevBtn').click();
    }
}

window.addEventListener('resize', () => updateCarousel(true));

document.querySelectorAll('details').forEach((el) => {
    const summary = el.querySelector('summary');
    const content = el.querySelector('.assembly-container');

    summary.addEventListener('click', (e) => {
        e.preventDefault();
        if (el.hasAttribute('open')) {
            const startHeight = content.offsetHeight;
            content.style.height = `${startHeight}px`;
            requestAnimationFrame(() => {
                content.style.height = '0px';
                content.style.opacity = '0';
                content.style.paddingTop = '0';
                content.style.paddingBottom = '0';
                content.style.borderTopWidth = '0';
            });
            content.addEventListener('transitionend', function onEnd() {
                el.removeAttribute('open');
                content.style.removeProperty('height');
                content.style.removeProperty('opacity');
                content.style.removeProperty('padding-top');
                content.style.removeProperty('padding-bottom');
                content.style.removeProperty('border-top-width');
                content.removeEventListener('transitionend', onEnd);
            }, { once: true });
        } else {
            el.setAttribute('open', '');
            const endHeight = content.scrollHeight;
            content.style.height = '0px';
            content.style.opacity = '0';
            content.style.paddingTop = '0';
            content.style.paddingBottom = '0';
            content.style.borderTopWidth = '0';
            
            requestAnimationFrame(() => {
                content.style.height = `${endHeight}px`;
                content.style.opacity = '1';
                content.style.paddingTop = '15px';
                content.style.paddingBottom = '15px';
                content.style.borderTopWidth = '1px';
            });
            
            content.addEventListener('transitionend', function onEnd() {
                content.style.height = 'auto';
                content.style.removeProperty('opacity');
                content.style.removeProperty('padding-top');
                content.style.removeProperty('padding-bottom');
                content.style.removeProperty('border-top-width');
                content.removeEventListener('transitionend', onEnd);
            }, { once: true });
        }
    });
});

const REPO = "VladimirGitsarev/Tinytosh";
async function fetchLatestRelease() {
    const loading = document.getElementById('loading-text');
    try {
        const response = await fetch(`https://api.github.com/repos/${REPO}/releases/latest`);
        if (!response.ok) throw new Error("Network response was not ok");
        
        const data = await response.json();
        
        const winAsset = data.assets.find(a => a.name.endsWith('.exe') || a.name.endsWith('.msi'));
        const macAsset = data.assets.find(a => a.name.endsWith('.dmg'));
        const linAsset = data.assets.find(a => a.name.endsWith('.deb') || a.name.endsWith('.AppImage'));

        if (winAsset) {
            const btn = document.getElementById('btn-win');
            btn.href = winAsset.browser_download_url;
            btn.title = `Download ${winAsset.name}`;
        } else {
            document.getElementById('btn-win').classList.add('btn-disabled');
            document.getElementById('btn-win').innerText = "Win (Not Found)";
        }

        if (macAsset) {
            const btn = document.getElementById('btn-mac');
            btn.href = macAsset.browser_download_url;
            btn.title = `Download ${macAsset.name}`;
        } else {
            document.getElementById('btn-mac').classList.add('btn-disabled');
            document.getElementById('btn-mac').innerText = "Mac (Not Found)";
        }

        if (linAsset) {
            const btn = document.getElementById('btn-linux');
            btn.href = linAsset.browser_download_url;
            btn.title = `Download ${linAsset.name}`;
        } else {
            document.getElementById('btn-linux').classList.add('btn-disabled');
            document.getElementById('btn-linux').innerText = "Linux (Not Found)";
        }

        loading.innerHTML = `Latest Version: <strong>${data.tag_name}</strong>`;
        loading.style.color = "var(--accent)";

    } catch (error) {
        console.error("Failed to fetch release:", error);
        loading.innerText = "Could not fetch direct links. Please verify on GitHub.";
    }
}

fetchLatestRelease();