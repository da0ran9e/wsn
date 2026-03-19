\documentclass[conference]{IEEEtran}
\IEEEoverridecommandlockouts
% The preceding line is only needed to identify funding in the first footnote. If that is unneeded, please comment it out.
\usepackage{cite}
\usepackage{amsmath,amssymb,amsfonts}
\usepackage{algorithmic}
\usepackage{graphicx}
\usepackage{textcomp}
\usepackage{xcolor}
\def\BibTeX{{\rm B\kern-.05em{\sc i\kern-.025em b}\kern-.08em
    T\kern-.1667em\lower.7ex\hbox{E}\kern-.125emX}}
\begin{document}

\title{Conference Paper Title*\\
{\footnotesize \textsuperscript{*}Note: Sub-titles are not captured in Xplore and
should not be used}
\thanks{Identify applicable funding agency here. If none, delete this.}
}

\author{\IEEEauthorblockN{1\textsuperscript{st} Given Name Surname}
\IEEEauthorblockA{\textit{dept. name of organization (of Aff.)} \\
\textit{name of organization (of Aff.)}\\
City, Country \\
email address or ORCID}
\and
\IEEEauthorblockN{2\textsuperscript{nd} Given Name Surname}
\IEEEauthorblockA{\textit{dept. name of organization (of Aff.)} \\
\textit{name of organization (of Aff.)}\\
City, Country \\
email address or ORCID}
\and
\IEEEauthorblockN{3\textsuperscript{rd} Given Name Surname}
\IEEEauthorblockA{\textit{dept. name of organization (of Aff.)} \\
\textit{name of organization (of Aff.)}\\
City, Country \\
email address or ORCID}
\and
\IEEEauthorblockN{4\textsuperscript{th} Given Name Surname}
\IEEEauthorblockA{\textit{dept. name of organization (of Aff.)} \\
\textit{name of organization (of Aff.)}\\
City, Country \\
email address or ORCID}
\and
\IEEEauthorblockN{5\textsuperscript{th} Given Name Surname}
\IEEEauthorblockA{\textit{dept. name of organization (of Aff.)} \\
\textit{name of organization (of Aff.)}\\
City, Country \\
email address or ORCID}
\and
\IEEEauthorblockN{6\textsuperscript{th} Given Name Surname}
\IEEEauthorblockA{\textit{dept. name of organization (of Aff.)} \\
\textit{name of organization (of Aff.)}\\
City, Country \\
email address or ORCID}
}

\maketitle

\begin{abstract}
This document is a model and instructions for \LaTeX.
This and the IEEEtran.cls file define the components of your paper [title, text, heads, etc.]. *CRITICAL: Do Not Use Symbols, Special Characters, Footnotes, 
or Math in Paper Title or Abstract.
\end{abstract}

\begin{IEEEkeywords}
% --- Paper skeleton (sections only) -----------------------------
\begin{abstract}
% Abstract (placeholder)
\end{abstract}

\begin{IEEEkeywords}
% keywords (placeholder)
\end{IEEEkeywords}

\section{Introduction}
Trong bối cảnh đô thị thông minh, nhu cầu truy vết và nhận diện nhanh đối tượng bị truy nã từ mạng camera IoT ngày càng cấp thiết. Tuy nhiên, dữ liệu nhận diện (ảnh/video chất lượng cao) thường có kích thước lớn, trong khi các nút biên có tài nguyên tính toán và băng thông hạn chế. Nếu UAV dừng tại từng nút để truyền toàn bộ dữ liệu, thời gian nhiệm vụ tăng mạnh và khó đáp ứng yêu cầu phát hiện sớm theo thời gian thực.

Bài báo này xem UAV như một ``data ferry'' bay qua khu vực khả nghi để phát quảng bá dữ liệu nhận diện theo từng mảnh (fragment). Thay vì yêu cầu mỗi nút phải nhận trọn bộ dữ liệu, chúng tôi sử dụng cơ chế tích lũy độ tin cậy theo xác suất: mỗi fragment đóng góp một phần bằng chứng, và nút sẽ phát cảnh báo khi mức tin cậy vượt ngưỡng. Cơ chế hợp tác nội ô (intra-cell cooperation) được kích hoạt theo ngưỡng trung gian để các nút trao đổi fragment còn thiếu ngay trong khi UAV đang bay, giúp rút ngắn thời gian đến cảnh báo đầu tiên.

Ở lớp truyền dẫn, nghiên cứu không chỉ dựa trên mô hình ``in-range'' tức thời mà tích hợp chuỗi quyết định gói tin theo hướng thực tế hơn: suy hao theo hình học không-đối-đất (air-to-ground), shadowing/fading, kiểm tra ``contact-window'' trên toàn bộ thời lượng phát gói, và ánh xạ SNR $\rightarrow$ BER $\rightarrow$ PER để quyết định mất gói ở mức xác suất. Nhờ đó, đánh giá thuật toán đường bay phản ánh đúng hơn các ràng buộc PHY trong môi trường đô thị.

Để tối ưu lộ trình UAV, chúng tôi xây dựng heuristic ``Greedy Max-Coverage with Cost'' (GMC), cân bằng lợi ích phủ sóng và chi phí di chuyển qua hàm điểm $\text{score}=\frac{\text{gain}}{\text{cost}^{\alpha}+\varepsilon}$. Tập ứng viên waypoint gồm vị trí nút nghi vấn và các centroid tùy chọn, cho phép bao phủ hiệu quả hơn so với chiến lược đi tuần tự lân cận gần nhất.

Các đóng góp chính của bài báo gồm: (i) mô hình phát hiện phân mảnh theo độ tin cậy tích lũy kết hợp hợp tác nội ô thời gian thực; (ii) pipeline PHY mức gói gắn với chuyển động UAV để đánh giá khả năng nhận gói theo thời gian; và (iii) thuật toán GMC cho bài toán cân bằng độ phủ--chi phí trong nhiệm vụ phát hiện khẩn cấp. Trên nền ns-3 mở rộng, chúng tôi cho thấy cách tiếp cận này có tiềm năng giảm thời gian phát hiện đầu tiên trong khi vẫn duy trì tính khả thi triển khai.

Phần còn lại của bài báo được tổ chức như sau: Mục II tổng quan công trình liên quan; Mục III mô tả mô hình hệ thống; Mục IV phát biểu bài toán; Mục V trình bày phương pháp đề xuất; Mục VI đánh giá hiệu năng; Mục VII thảo luận giới hạn; và Mục VIII kết luận.

\section{Related Work}
% Summarize related literature (skeleton only).

\section{System Model}
% Describe network/model assumptions and notation.
% - Thiết lập mạng: Mô tả cách các edge node được triển khai trong khu vực đô thị và các thông số liên quan đến vị trí và phạm vi hoạt động của UAV.
% - Mô hình hệ thống: Khu vực mạng được chia thành các cell, mỗi cell chứa một số nút biên. Mỗi nút có khả năng nhận diện đối tượng sử dụng fragments. Khu vực khả nghi được BS xác định trong đó có khả năng xuất hiện đối tượng bị truy nã.
% - Mô hình lập kế hoạch quỹ đạo: UAV có vùng phủ sóng giới hạn và phải lên kế hoạch quỹ đạo để tối ưu hóa việc phát dữ liệu đến các nút biên trong khu vực khả nghi.
% - Mô hình dữ liệu: Dữ liệu nhận diện được chia thành các fragment có kích thước nhỏ hơn, mỗi fragment chứa một phần thông tin về đối tượng bị truy nã. Mỗi fragment có xác suất được nhận thành công tại nút biên dựa trên mô hình truyền dẫn.
% - Mô hình phát tán dữ liệu: UAV phát dữ liệu theo từng fragment khi bay qua khu vực khả nghi. Các nút biên tích lũy độ tin cậy dựa trên số lượng fragment nhận được và có thể trao đổi fragment với nhau để tăng độ tin cậy.
% - Mô hình kênh truyền thông: Mô hình truyền dẫn air-to-ground bao gồm suy hao theo hình học, shadowing/fading, và xác suất mất gói dựa trên SNR $\rightarrow$ BER $\rightarrow$ PER.
% - Mô hình tốc độ truyền: Tốc độ truyền dữ liệu giữa UAV và nút biên phụ thuộc vào khoảng cách, điều kiện kênh, và có thể thay đổi theo thời gian khi UAV di chuyển.
Chúng tôi xét một mạng IoT đô thị gồm tập nút biên $\mathcal{N}$, một UAV và một trạm gốc (BS). Các nút biên được triển khai theo lưới trong khu vực giám sát, mỗi nút có khả năng nhận dạng cục bộ và lưu trữ fragment. BS thu thập topology, xác định vùng nghi vấn, tạo dữ liệu phân mảnh và điều phối nhiệm vụ bay của UAV.

\subsection{Thiết lập mạng và mô hình hệ thống}
Khu vực giám sát được chia thành các cell để hỗ trợ hợp tác nội ô. Mỗi cell có một tập node thành viên, trong đó một node có thể đóng vai trò điều phối cục bộ (cell leader) trong quá trình chia sẻ fragment. Từ thông tin topology ban đầu, BS xác định tập nút nghi vấn $\mathcal{P} \subseteq \mathcal{N}$, tương ứng vùng có xác suất cao xuất hiện đối tượng truy nã. Trong mô hình này, một node đạt phát hiện khi độ tin cậy tích lũy vượt ngưỡng cảnh báo.

\subsection{Mô hình lập kế hoạch quỹ đạo UAV}
UAV bay ở độ cao cố định với vùng phủ sóng quảng bá hữu hạn $R_b$, và phải chọn dãy waypoint để phủ tập $\mathcal{P}$. Chúng tôi sử dụng heuristic Greedy Max-Coverage with Cost (GMC), chọn waypoint tối đa hóa tỉ số giữa lợi ích phủ mới và chi phí di chuyển:
\begin{equation}
\text{score}(c)=\frac{\left|\mathrm{CS}(c)\setminus \mathrm{Covered}\right|}{\left(d(x_t,c)/v\right)^{\alpha}+\varepsilon},
\end{equation}
trong đó $\mathrm{CS}(c)$ là tập nút nghi vấn nằm trong bán kính phủ của ứng viên $c$, $d(x_t,c)$ là khoảng cách từ vị trí hiện tại của UAV đến $c$, và $v$ là vận tốc bay.

\subsection{Mô hình dữ liệu và phát tán fragment}
Dữ liệu nhận diện gốc được chia thành $K$ fragment; UAV phát tuần tự các fragment trong khi di chuyển, thay vì dừng để truyền toàn bộ dữ liệu cho từng nút. Mỗi node $n$ duy trì tập fragment đã nhận $\mathcal{F}_n$ và tính độ tin cậy tích lũy theo mô hình hợp nhất xác suất:
\begin{equation}
C_n = 1-\prod_{i\in\mathcal{F}_n}(1-p_i),
\end{equation}
với $p_i$ là mức đóng góp nhận dạng của fragment $i$. Khi $C_n$ vượt ngưỡng hợp tác, node có thể yêu cầu/trao đổi fragment còn thiếu trong cell; khi $C_n$ vượt ngưỡng cảnh báo, node phát cảnh báo lên BS.

\subsection{Mô hình kênh truyền và tốc độ truyền}
Liên kết UAV--node sử dụng mô hình air-to-ground theo hình học, bao gồm suy hao khoảng cách, shadowing và fast fading. Công suất thu tại thời điểm $t$ được biểu diễn dạng tổng quát:
\begin{equation}
P_{rx}(t)=P_{tx}-PL(d_t)-X_{\sigma}(t)-X_f(t),
\end{equation}
trong đó $d_t$ biến thiên theo chuyển động UAV. Từ SNR tức thời, hệ thống ánh xạ sang BER và PER để quyết định mất gói ở mức packet:
\begin{equation}
\mathrm{PER}=1-(1-\mathrm{BER})^{8L}.
\end{equation}
Ngoài ra, điều kiện nhận gói còn phụ thuộc contact-window: liên kết phải duy trì trên ngưỡng thu trong toàn bộ thời lượng truyền gói. Do đó, tốc độ truyền hiệu dụng giữa UAV và node không cố định mà thay đổi theo khoảng cách, profile kênh và trạng thái chuyển động theo thời gian.


\section{Problem Statement}
% Formal problem definition and objectives.
% - Mục tiêu: tối đa hóa thời gian phát hiện đầu tiên của đối tượng bị truy nã tại các nút biên trong khu vực khả nghi.
% - Ràng buộc: UAV có vùng phủ sóng giới hạn, dữ liệu nhận diện được chia thành các fragment, và các nút biên có tài nguyên hạn chế để nhận và xử lý dữ liệu.
% - Bài toán: Lập kế hoạch quỹ đạo cho UAV để phát dữ liệu nhận và trao đổi dữ liệu giữa các nút biên nhằm đạt được độ tin cậy tích lũy vượt ngưỡng trong thời gian ngắn nhất có thể.
Cho tập nút nghi vấn $\mathcal{P}$ do BS xác định, UAV cần phát tập fragment $\mathcal{F}=\{1,\ldots,K\}$ trong khi di chuyển để một nút trong $\mathcal{P}$ đạt ngưỡng cảnh báo sớm nhất. Bài toán kết hợp hai quyết định đồng thời: (i) quỹ đạo UAV qua chuỗi waypoint và (ii) lịch phát fragment theo thời gian.

Ký hiệu $\Pi=(w_1,w_2,\ldots,w_M)$ là quỹ đạo UAV, với $w_m$ là waypoint thứ $m$, và $\mathcal{S}=\{(f,\tau_f)\}$ là lịch phát fragment $f$ tại thời điểm $\tau_f$. Gọi $C_n(t)$ là độ tin cậy tích lũy tại node $n$ ở thời điểm $t$, và $\tau_{\text{alert}}$ là ngưỡng cảnh báo. Thời điểm phát hiện đầu tiên được định nghĩa:
\begin{equation}
T_{\text{detect}}(\Pi,\mathcal{S}) = \inf\{t\ge 0\;|\;\exists n\in\mathcal{P}: C_n(t)\ge \tau_{\text{alert}}\}.
\end{equation}

Mục tiêu tối ưu là rút ngắn thời gian phát hiện kỳ vọng:
\begin{equation}
\min_{\Pi,\mathcal{S}}\;\mathbb{E}[T_{\text{detect}}(\Pi,\mathcal{S})].
\end{equation}

Bài toán chịu các ràng buộc vận hành sau:
\begin{equation}
d(w_m,w_{m+1}) \le v_{\max}\,\Delta t_m,\;\forall m,
\end{equation}
\begin{equation}
\sum_{m=1}^{M-1}\frac{d(w_m,w_{m+1})}{v}+\sum_f t_f^{\text{tx}} \le T_{\max},
\end{equation}
\begin{equation}
\mathbf{1}_{\{n\leftarrow f\}}(t)=1 \Rightarrow P_{rx}^{(n)}(t')\ge P_{\text{sens}},\;\forall t'\in[t,t+t_f^{\text{tx}}],
\end{equation}
\begin{equation}
\Pr[\text{packet error}|n,f,t]=\mathrm{PER}(\mathrm{SNR}_{n,f}(t),L_f),
\end{equation}
\begin{equation}
\sum_{f\in\mathcal{F}_n(t)} b_f \le B_n^{\max},\;\forall n\in\mathcal{P},
\end{equation}
trong đó $v_{\max}$ là tốc độ bay cực đại, $T_{\max}$ là thời hạn nhiệm vụ, $P_{\text{sens}}$ là ngưỡng thu, $L_f$ và $b_f$ lần lượt là kích thước gói và dữ liệu fragment, còn $B_n^{\max}$ là giới hạn bộ đệm tại node.

Do bài toán có bản chất tổ hợp (tối ưu quỹ đạo rời rạc, lịch phát theo thời gian và ràng buộc kênh ngẫu nhiên), nghiệm tối ưu toàn cục khó đạt theo thời gian thực. Vì vậy, phần tiếp theo đề xuất chiến lược heuristic GMC để xấp xỉ bài toán và duy trì cân bằng giữa độ phủ, chi phí di chuyển và xác suất nhận thành công ở mức gói.

\section{Proposed Approach}
\subsection{Overview}
% High-level description of the proposed method.
\subsection{Algorithm}
% Pseudocode / algorithm sketch.

\section{Performance Evaluation}
\subsection{Simulation Setup}
% Describe simulation environment and parameters.
\subsection{Metrics}
% Define evaluation metrics.
\subsection{Results}
% Placeholder for figures/tables and result discussion.

\section{Discussion}
% Limitations and interpretation.

\section{Conclusion}
% Summary and future work.

\section*{Acknowledgment}
% Acknowledgments (if any).

\bibliographystyle{IEEEtran}
\bibliography{references}

\appendix
\section{Appendix: Additional Material}
% Optional appendices.

\end{document}
\end{equation}
