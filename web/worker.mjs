function parseColor(color) {
  const r = ((color >> (8 * 0)) & 0xff).toString(16).padStart(2, 0);
  const g = ((color >> (8 * 1)) & 0xff).toString(16).padStart(2, 0);
  const b = ((color >> (8 * 2)) & 0xff).toString(16).padStart(2, 0);
  const a = ((color >> (8 * 3)) & 0xff).toString(16).padStart(2, 0);

  return "#" + r + g + b + a;
}

function getColorValue(instance, color) {
  return new Uint32Array(instance.exports.memory.buffer, color, 1)[0];
}

let updateFrameIndex = null;

onmessage = async (event) => {
  const canvas = event.data.canvas;
  const ctx = canvas.getContext("2d");

  const { instance } = await WebAssembly.instantiateStreaming(
    fetch("./main.wasm"),
    {
      env: {
        set_canvas_size: function (w, h) {
          ctx.canvas.width = w;
          ctx.canvas.height = h;
        },
        clear_with_color: function (colorPtr) {
          ctx.fillStyle = parseColor(getColorValue(instance, colorPtr));
          ctx.fillRect(0, 0, ctx.canvas.width, ctx.canvas.height);
        },
        fill_rect: function (x, y, w, h, colorPtr) {
          ctx.fillStyle = parseColor(getColorValue(instance, colorPtr));
          ctx.fillRect(x, y, w, h);
        },
        fill_circle: function (x, y, radius, colorPtr) {
          ctx.fillStyle = parseColor(getColorValue(instance, colorPtr));
          ctx.beginPath();
          ctx.arc(x, y, radius, 0, 2 * Math.PI);
          ctx.fill();
        },
        set_update_frame: function (fIndex) {
          updateFrameIndex = fIndex;
        },
      },
    },
  );

  instance.exports.run();

  const table =
    instance.exports.__indirect_function_table || instance.exports.table;
  const updateFrame = table.get(updateFrameIndex);

  function gameLoop(timestamp) {
    const deltaTime = (timestamp - previousTimestamp) * 0.001;
    previousTimestamp = timestamp;

    updateFrame(deltaTime);
    self.requestAnimationFrame(gameLoop);
  }

  self.requestAnimationFrame((timestamp) => {
    previousTimestamp = timestamp;
    gameLoop(timestamp);
  });
};
