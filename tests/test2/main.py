import sys, re
from typing import Optional

from PyQt6.QtWidgets import (
    QApplication, QWidget, QLabel, QPushButton, QVBoxLayout, QHBoxLayout, QSizePolicy
)
from PyQt6.QtGui import QPainter, QPainterPath, QPen, QBrush, QPixmap, QImage
from PyQt6.QtCore import Qt, QPointF, QRectF, QSize, QRect
from PyQt6.QtSvg import QSvgGenerator, QSvgRenderer

APP_STYLE = """
QWidget { background: #f6f7fb; font-size: 14px; color: #222; }
QLabel#title { font-size: 16px; font-weight: 600; padding: 4px 0 8px 0; }
QPushButton {
  padding: 8px 14px; border: 1px solid #bfc7d5; border-radius: 6px; background: white;
}
QPushButton:hover { background: #fafafa; }
QPushButton:pressed { background: #f0f0f0; }
#Card { background: white; border: 1px solid #d7dbe6; border-radius: 10px; }
"""

SQUARE_SIZE = 400   # inner drawable square (pixels)
MARGIN = 12         # border inset inside each card


class FramedCanvas(QWidget):
    """White card with a black bordered drawing area."""
    def __init__(self, for_output: bool = False):
        super().__init__()
        self.setObjectName("Card")
        self.for_output = for_output
        self.margin = MARGIN
        outer_w = SQUARE_SIZE + self.margin * 2
        outer_h = SQUARE_SIZE + self.margin * 2
        self.setFixedSize(outer_w, outer_h)

        # Single path; new press clears the old one instead of joining
        self.path: Optional[QPainterPath] = None
        self.is_drawing = False
        self.pen = QPen(Qt.GlobalColor.black, 3)
        self.pix: Optional[QPixmap] = None  # for output

    def drawing_rect(self) -> QRectF:
        return QRectF(self.margin, self.margin, SQUARE_SIZE, SQUARE_SIZE)

    # -------- mouse handling (input mode only) --------
    def mousePressEvent(self, e):
        if self.for_output or e.button() != Qt.MouseButton.LeftButton:
            return
        pt = QPointF(e.position())
        if not self.drawing_rect().contains(pt):
            return
        # start a NEW path every press (clears old one)
        self.path = QPainterPath(pt)
        self.is_drawing = True
        self.update()

    def mouseMoveEvent(self, e):
        if self.for_output or not self.is_drawing or self.path is None:
            return
        if not (e.buttons() & Qt.MouseButton.LeftButton):
            return
        pt = QPointF(e.position())
        if self.drawing_rect().contains(pt):
            self.path.lineTo(pt)
            self.update()

    def mouseReleaseEvent(self, e):
        if self.for_output:
            return
        if e.button() == Qt.MouseButton.LeftButton:
            self.is_drawing = False

    # -------- painting --------
    def paintEvent(self, _):
        p = QPainter(self)
        p.setRenderHint(QPainter.RenderHint.Antialiasing)
        p.fillRect(self.rect(), QBrush(Qt.GlobalColor.white))
        p.setPen(QPen(Qt.GlobalColor.black, 2))
        p.drawRect(self.drawing_rect())
        if self.for_output:
            if self.pix:
                rect = self.drawing_rect().toRect()
                scaled = self.pix.scaled(
                    rect.size(),
                    Qt.AspectRatioMode.KeepAspectRatio,
                    Qt.TransformationMode.SmoothTransformation,
                )
                # Precise centering (no left bias)
                target = QRect(0, 0, scaled.width(), scaled.height())
                target.moveCenter(rect.center())
                p.drawPixmap(target, scaled)
        else:
            if self.path is not None:
                p.setPen(self.pen)
                p.drawPath(self.path)

    # -------- export drawing to SVG (just the inner square) --------
    def export_svg(self, filename: str):
        inner = self.drawing_rect()
        gen = QSvgGenerator()
        gen.setFileName(filename)
        gen.setSize(QSize(int(inner.width()), int(inner.height())))
        gen.setViewBox(inner.toRect())
        p = QPainter(gen)
        p.translate(-inner.x(), -inner.y())
        p.setPen(self.pen)
        if self.path is not None:
            p.drawPath(self.path)
        p.end()

    # -------- set output from an SVG file --------
    def set_svg_output(self, svg_path: str):
        renderer = QSvgRenderer(svg_path)
        size = renderer.defaultSize()
        if not size.isValid():
            size = QSize(SQUARE_SIZE, SQUARE_SIZE)
        img = QImage(size, QImage.Format.Format_ARGB32_Premultiplied)
        img.fill(0)
        p = QPainter(img)
        renderer.render(p)
        p.end()
        self.pix = QPixmap.fromImage(img)
        self.update()

    def clear(self):
        self.path = None
        self.is_drawing = False
        self.update()


class SimpleApp(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Draw your image")

        self.input_title = QLabel("draw your image", alignment=Qt.AlignmentFlag.AlignCenter)
        self.input_title.setObjectName("title")
        self.input_canvas = FramedCanvas(for_output=False)

        self.output_title = QLabel("processed output (loop closed)", alignment=Qt.AlignmentFlag.AlignCenter)
        self.output_title.setObjectName("title")
        self.output_canvas = FramedCanvas(for_output=True)

        self.btn_process = QPushButton("Process and Show")
        self.btn_clear = QPushButton("Clear")

        # ---- layout ----
        left_col = QVBoxLayout()
        left_col.addWidget(self.input_title)
        left_col.addWidget(self.input_canvas)

        right_col = QVBoxLayout()
        right_col.addWidget(self.output_title)
        right_col.addWidget(self.output_canvas)

        top_row = QHBoxLayout()
        top_row.setSpacing(20)
        top_row.addLayout(left_col)
        top_row.addLayout(right_col)

        btn_row = QHBoxLayout()
        btn_row.setSpacing(10)
        btn_row.addStretch(1)
        btn_row.addWidget(self.btn_process)
        btn_row.addWidget(self.btn_clear)
        btn_row.addStretch(1)

        root = QVBoxLayout(self)
        root.setContentsMargins(20, 20, 20, 20)
        root.setSpacing(20)
        root.addLayout(top_row)
        root.addSpacing(10)
        root.addLayout(btn_row)

        # wiring
        self.btn_process.clicked.connect(self.process_and_show)
        self.btn_clear.clicked.connect(self.clear_canvas)

    def clear_canvas(self):
        self.input_canvas.clear()
        self.output_canvas.pix = None
        self.output_canvas.update()

    def process_and_show(self):
        """
        Export SVG, then minimally "process":
          - Do NOT change colors/widths.
          - Close the first <path> d=... by appending 'Z' (loop).
          - No XML parser; simple regex on the first path's d attribute.
        """
        svg_in = "drawing.svg"
        svg_out = "processed.svg"
        self.input_canvas.export_svg(svg_in)

        txt = open(svg_in, "r", encoding="utf-8").read()

        # Regex to close ONLY the first d="...": append ' Z' before the closing quote if not already closed
        def close_first_path_d(m):
            d = m.group(1)
            if re.search(r'[Zz]\s*$', d):
                return f'd="{d}"'
            else:
                return f'd="{d} Z"'

        txt_closed = re.sub(r'd="([^"]*?)"', lambda m: close_first_path_d(m), txt, count=1)

        open(svg_out, "w", encoding="utf-8").write(txt_closed)
        self.output_canvas.set_svg_output(svg_out)


if __name__ == "__main__":
    app = QApplication(sys.argv)
    app.setStyleSheet(APP_STYLE)
    w = SimpleApp()
    window_width = (SQUARE_SIZE + MARGIN * 2) * 2 + 60
    window_height = (SQUARE_SIZE + MARGIN * 2) + 160
    w.setFixedSize(window_width, window_height)
    w.show()
    sys.exit(app.exec())
